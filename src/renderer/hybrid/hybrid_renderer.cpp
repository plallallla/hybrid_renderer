#include "hybrid_renderer.h"

#include "../../core/random.h"
#include "../../geometry/ray_scene.h"
#include "../gbuffer.h"
#include "../material_evaluator.h"
#include "../pbr.h"
#include "../rasterizer/rasterizer.h"
#include "../trace_primitive_builder.h"

#include <atomic>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

namespace hr
{
namespace
{
Vec3 RandomUnitVector(Random& rng)
{
    const float z = rng.NextFloat() * 2.0f - 1.0f;
    const float a = rng.NextFloat() * TWO_PI;
    const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    return {r * std::cos(a), r * std::sin(a), z};
}

// Cosine-weighted direction in the hemisphere around N.
Vec3 SampleHemisphere(const Vec3& N, Random& rng)
{
    const Vec3 dir = N + RandomUnitVector(rng);
    const float lenSq = Dot(dir, dir);
    if (lenSq < 1e-12f)
    {
        return N;
    }
    return Normalize(dir);
}

// Build a MaterialSample directly from the scene material (no texture / normal-
// map evaluation): used for shading the surface a reflection ray lands on.
MaterialSample SampleFromMaterial(const Scene& scene, int materialID, const Vec3& normal)
{
    MaterialSample sample;
    if (materialID >= 0 && materialID < static_cast<int>(scene.materials.size()))
    {
        const Material& material = scene.materials[static_cast<size_t>(materialID)];
        sample.baseColor = material.baseColor;
        sample.metallic = material.metallic;
        sample.roughness = Clamp(material.roughness, 0.04f, 1.0f);
        sample.emission = material.emission;
    }
    sample.normal = Normalize(normal);
    return sample;
}

// Direct PBR lighting over all lights plus the constant ambient fill. When
// `rayScene` is non-null, each light is shadowed via an occlusion query
// (Hybrid V1). `ao` modulates only the ambient term (Hybrid V2 feeds the
// ray-traced factor here).
Vec3 ComposeLighting(
    const Scene& scene,
    const MaterialSample& sample,
    const Vec3& P,
    const Vec3& V,
    float ao,
    const RayScene* rayScene,
    Random& rng,
    int areaSamples)
{
    const Vec3 N = sample.normal;
    Vec3 color{0.0f, 0.0f, 0.0f};

    for (const DirectionalLight& light : scene.lights)
    {
        const Vec3 L = Normalize(-light.direction);
        float visibility = 1.0f;
        if (rayScene != nullptr)
        {
            const Ray shadowRay{P + N * EPSILON, L};
            if (rayScene->Occluded(shadowRay, EPSILON, std::numeric_limits<float>::max()))
            {
                visibility = 0.0f;
            }
        }
        color = color + EvaluatePBRDirect(sample, N, V, L, light.color * light.intensity) * visibility;
    }

    for (const PointLight& light : scene.pointLights)
    {
        const Vec3 toLight = light.position - P;
        const float distance = Length(toLight);
        const Vec3 L = Normalize(toLight);
        float visibility = 1.0f;
        if (rayScene != nullptr)
        {
            const Ray shadowRay{P + N * EPSILON, L};
            if (rayScene->Occluded(shadowRay, EPSILON, distance - EPSILON))
            {
                visibility = 0.0f;
            }
        }
        color = color + EvaluatePBRDirect(sample, N, V, L, light.color * light.intensity) * visibility;
    }

    // Rectangular area lights: Monte-Carlo estimate over the light surface, with
    // a shadow ray per sample. Averaging visibility across samples is what
    // produces soft penumbrae (V1 soft shadows). The light radiance fed to
    // EvaluatePBRDirect already folds in the light-side cosine, area and inverse
    // square distance (1/area pdf cancels the area), so the BRDF's own NdotL is
    // the remaining surface-side cosine.
    for (const AreaLight& light : scene.areaLights)
    {
        const Vec3 nL = light.Normal();
        const Vec3 Le = light.Radiance();
        const float area = light.Area();
        const int samples = std::max(1, areaSamples);
        Vec3 sum{0.0f, 0.0f, 0.0f};
        for (int s = 0; s < samples; ++s)
        {
            const float ru = rng.NextFloat() * 2.0f - 1.0f;
            const float rv = rng.NextFloat() * 2.0f - 1.0f;
            const Vec3 Q = light.center + light.uVec * ru + light.vVec * rv;
            const Vec3 toLight = Q - P;
            const float distance = Length(toLight);
            if (distance < EPSILON)
            {
                continue;
            }
            const Vec3 L = toLight / distance;
            const float cosLight = std::abs(Dot(nL, L));
            if (cosLight <= 0.0f)
            {
                continue;
            }
            float visibility = 1.0f;
            if (rayScene != nullptr)
            {
                const Ray shadowRay{P + N * EPSILON, L};
                if (rayScene->Occluded(shadowRay, EPSILON, distance - EPSILON))
                {
                    visibility = 0.0f;
                }
            }
            const Vec3 radiance = Le * (cosLight * area / (distance * distance));
            sum = sum + EvaluatePBRDirect(sample, N, V, L, radiance) * visibility;
        }
        color = color + sum * (1.0f / static_cast<float>(samples));
    }

    // Constant ambient fill (matches the rasterizer's 0.15 * baseColor * ao),
    // modulated by the ambient-occlusion factor.
    color = color + sample.baseColor * (sample.ao * ao * 0.15f);
    // Self-emission (so reflections of an emissive panel show the light).
    color = color + sample.emission;
    return color;
}
} // namespace

void HybridRenderer::Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings)
{
    const int width = settings.width;
    const int height = settings.height;

    // V1 step 1: rasterize primary visibility into the PBR GBuffer. Reuses the
    // software rasterizer rather than re-implementing rasterization (§21).
    SoftwareRasterizer rasterizer;
    GBuffer gbuffer;
    rasterizer.RenderGBuffer(scene, camera, gbuffer, settings);

    // Secondary effects query a RayScene built from the same scene geometry.
    RayScene rayScene;
    rayScene.Build(BuildTracePrimitives(scene));
    rayScene.SetUseBVH(settings.useBVH);

    output.Resize(width, height);
    output.Clear(scene.background);

    const RayScene* shadowScene = settings.enableRayTracedShadow ? &rayScene : nullptr;
    const int aoSamples = std::max(1, settings.aoSamples);

    std::atomic<int> nextRow{0};
    auto shadeRows = [&]()
    {
        int y = 0;
        while ((y = nextRow.fetch_add(1)) < height)
        {
            for (int x = 0; x < width; ++x)
            {
                // Image<bool> wraps std::vector<bool>, whose element access
                // yields a proxy rather than bool&, so read coverage through
                // Data() (the rasterizer writes it the same way).
                const bool covered = gbuffer.valid.Data()[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)];
                if (!covered)
                {
                    output.color.At(x, y) = scene.background;
                    continue;
                }

                // Emissive panels (area lights) are shown directly and skip
                // shading entirely — they emit rather than receive light, and
                // self-lighting a coincident surface would produce fireflies.
                const int materialId = gbuffer.materialID.At(x, y);
                if (materialId >= 0 && materialId < static_cast<int>(scene.materials.size()))
                {
                    const Vec3 emission = scene.materials[static_cast<size_t>(materialId)].emission;
                    if (emission.x > 0.0f || emission.y > 0.0f || emission.z > 0.0f)
                    {
                        output.color.At(x, y) = Clamp(emission, 0.0f, 1.0f);
                        continue;
                    }
                }

                const Vec3 P = gbuffer.position.At(x, y);
                MaterialSample sample;
                sample.baseColor = gbuffer.baseColor.At(x, y);
                sample.normal = Normalize(gbuffer.normal.At(x, y));
                sample.metallic = gbuffer.metallic.At(x, y);
                sample.roughness = gbuffer.roughness.At(x, y);
                const Vec3 N = sample.normal;
                const Vec3 V = Normalize(camera.position - P);

                Random rng(static_cast<uint32_t>(y) * static_cast<uint32_t>(width) + static_cast<uint32_t>(x) + 1u);

                // V2: ray-traced ambient occlusion (modulates ambient only).
                float ao = 1.0f;
                if (settings.enableRayTracedAO)
                {
                    int occluded = 0;
                    for (int i = 0; i < aoSamples; ++i)
                    {
                        const Vec3 dir = SampleHemisphere(N, rng);
                        const Ray aoRay{P + N * EPSILON, dir};
                        if (rayScene.Occluded(aoRay, EPSILON, settings.aoRadius))
                        {
                            ++occluded;
                        }
                    }
                    ao = 1.0f - static_cast<float>(occluded) / static_cast<float>(aoSamples);
                }

                Vec3 color = ComposeLighting(scene, sample, P, V, ao, shadowScene, rng, settings.areaLightSamples);

                // V3: ray-traced mirror reflection, Fresnel-weighted.
                if (settings.enableRayTracedReflection)
                {
                    const Vec3 R = Reflect(-V, N);
                    const Ray reflectionRay{P + N * EPSILON, R};
                    HitRecord hit;
                    Vec3 reflectedRadiance = scene.background;
                    if (rayScene.Intersect(reflectionRay, hit, EPSILON, std::numeric_limits<float>::max()))
                    {
                        const MaterialSample reflectedSample = SampleFromMaterial(scene, hit.materialID, hit.normal);
                        const Vec3 reflectedV = Normalize(P - hit.position);
                        // One bounce, shadowed by the same scene so reflected
                        // surfaces still respect occlusion.
                        reflectedRadiance = ComposeLighting(scene, reflectedSample, hit.position, reflectedV, 1.0f, shadowScene, rng, settings.areaLightSamples);
                    }

                    const Vec3 F0 = Lerp(Vec3{0.04f, 0.04f, 0.04f}, sample.baseColor, sample.metallic);
                    const Vec3 F = FresnelSchlick(std::max(0.0f, Dot(N, V)), F0);
                    const float reflectionWeight = 1.0f - sample.roughness;
                    color = color + F * reflectedRadiance * reflectionWeight;
                }

                // Match the rasterizer's output convention (clamp, no gamma) so
                // raster and hybrid of the same scene are directly comparable.
                output.color.At(x, y) = Clamp(color, 0.0f, 1.0f);
            }
        }
    };

    const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(shadeRows);
    }
    for (std::thread& thread : threads)
    {
        thread.join();
    }
}
} // namespace hr
