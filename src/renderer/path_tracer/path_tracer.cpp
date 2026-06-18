#include "path_tracer.h"

#include "../pbr.h"
#include "../tone_mapping.h"
#include "../trace_primitive_builder.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

namespace hr
{
namespace
{
// Uniformly distributed point on the unit sphere (RTIOW-style). Used both for
// the Lambertian diffuse lobe (normal + unit vector = cosine-weighted) and to
// fuzz the metal reflection direction.
Vec3 RandomUnitVector(Random& rng)
{
    const float z = rng.NextFloat() * 2.0f - 1.0f;
    const float a = rng.NextFloat() * TWO_PI;
    const float r = std::sqrt(std::max(0.0f, 1.0f - z * z));
    return {r * std::cos(a), r * std::sin(a), z};
}

bool NearZero(const Vec3& v)
{
    const float s = 1e-8f;
    return std::abs(v.x) < s && std::abs(v.y) < s && std::abs(v.z) < s;
}

Vec3 EstimateDirectAreaLight(
    const Scene& scene,
    const RayScene& rayScene,
    const HitRecord& hit,
    const Material& material,
    const Vec3& viewDir,
    Random& rng,
    const RenderSettings& settings)
{
    MaterialSample sample;
    sample.baseColor = material.baseColor;
    sample.normal = hit.normal;
    sample.metallic = material.metallic;
    sample.roughness = Clamp(material.roughness, 0.04f, 1.0f);

    Vec3 direct{0.0f, 0.0f, 0.0f};
    for (const AreaLight& light : scene.areaLights)
    {
        const Vec3 nL = light.Normal();
        const Vec3 Le = light.Radiance();
        const float area = light.Area();
        const int samples = std::max(1, settings.areaLightSamples);
        Vec3 sum{0.0f, 0.0f, 0.0f};

        for (int i = 0; i < samples; ++i)
        {
            const float ru = rng.NextFloat() * 2.0f - 1.0f;
            const float rv = rng.NextFloat() * 2.0f - 1.0f;
            const Vec3 q = light.center + light.uVec * ru + light.vVec * rv;
            const Vec3 toLight = q - hit.position;
            const float distance = Length(toLight);
            if (distance <= EPSILON)
            {
                continue;
            }

            const Vec3 L = toLight / distance;
            const float cosLight = std::abs(Dot(nL, L));
            if (cosLight <= 0.0f)
            {
                continue;
            }

            const Ray shadowRay{hit.position + hit.normal * EPSILON, L};
            if (rayScene.Occluded(shadowRay, EPSILON, distance - EPSILON))
            {
                continue;
            }

            const Vec3 radiance = Le * (cosLight * area / (distance * distance));
            sum = sum + EvaluatePBRDirect(sample, sample.normal, viewDir, L, radiance);
        }

        direct = direct + sum * (1.0f / static_cast<float>(samples));
    }
    return direct;
}
} // namespace

Vec3 PathTracer::TraceRay(const Scene& scene, const RayScene& rayScene, const Ray& ray, int depth, Random& rng, const RenderSettings& settings) const
{
    if (depth <= 0)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    HitRecord hit;
    if (!rayScene.Intersect(ray, hit, EPSILON, std::numeric_limits<float>::max()))
    {
        // Ray escaped the scene: sample the constant sky (Scene::background).
        return scene.background;
    }

    Material material;
    if (hit.materialID >= 0 && hit.materialID < static_cast<int>(scene.materials.size()))
    {
        material = scene.materials[hit.materialID];
    }

    // Emissive geometry remains hittable by bounce rays, while the explicit
    // area-light estimate below keeps closed Cornell-style rooms converging.
    const Vec3 emitted = material.emission;
    const Vec3 direct = Dot(emitted, emitted) > 0.0f
        ? Vec3{0.0f, 0.0f, 0.0f}
        : EstimateDirectAreaLight(scene, rayScene, hit, material, Normalize(-ray.direction), rng, settings);
    const Vec3 attenuation = material.baseColor;
    Vec3 scatterDir;

    if (material.metallic < 0.5f)
    {
        // Diffuse (Lambertian) bounce: cosine-weighted around the hit normal.
        scatterDir = hit.normal + RandomUnitVector(rng);
        if (NearZero(scatterDir))
        {
            scatterDir = hit.normal;
        }
        scatterDir = Normalize(scatterDir);
    }
    else
    {
        // Rough-metal reflection bounce, fuzzed by roughness.
        const Vec3 reflected = Reflect(Normalize(ray.direction), hit.normal);
        scatterDir = Normalize(reflected + RandomUnitVector(rng) * material.roughness);
        if (Dot(scatterDir, hit.normal) <= 0.0f)
        {
            // Scattered below the surface: absorbed (still counts emission).
            return emitted;
        }
    }

    const Ray scattered{hit.position + hit.normal * EPSILON, scatterDir};
    return emitted + direct + attenuation * TraceRay(scene, rayScene, scattered, depth - 1, rng, settings);
}

void PathTracer::Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings)
{
    const int width = settings.width;
    const int height = settings.height;
    output.Resize(width, height);

    RayScene rayScene;
    rayScene.Build(BuildTracePrimitives(scene));
    rayScene.SetUseBVH(settings.useBVH);

    // GenerateRay reads Camera::aspect, so make sure it matches the output.
    Camera cam = camera;
    cam.aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

    const int spp = std::max(1, settings.samplesPerPixel);
    const int maxDepth = std::max(1, settings.maxDepth);
    const float invSpp = 1.0f / static_cast<float>(spp);

    std::atomic<int> nextRow{0};
    auto renderRows = [&]()
    {
        int y = 0;
        while ((y = nextRow.fetch_add(1)) < height)
        {
            for (int x = 0; x < width; ++x)
            {
                // Per-pixel seed keeps the result deterministic regardless of
                // how rows are distributed across threads.
                Random rng(static_cast<uint32_t>(y) * static_cast<uint32_t>(width) + static_cast<uint32_t>(x) + 1u);

                Vec3 color{0.0f, 0.0f, 0.0f};
                for (int s = 0; s < spp; ++s)
                {
                    const float u = (static_cast<float>(x) + rng.NextFloat()) / static_cast<float>(width);
                    const float v = (static_cast<float>(y) + rng.NextFloat()) / static_cast<float>(height);
                    const Ray ray = cam.GenerateRay(u, v);
                    color = color + TraceRay(scene, rayScene, ray, maxDepth, rng, settings);
                }

                color = color * invSpp;
                color = ApplyExposure(color, settings.exposure);
                if (settings.enableGammaCorrection)
                {
                    color = GammaCorrect(color);
                }
                output.color.At(x, y) = color;
            }
        }
    };

    const unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(renderRows);
    }
    for (std::thread& thread : threads)
    {
        thread.join();
    }
}
} // namespace hr
