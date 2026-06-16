#include "path_tracer.h"

#include "../tone_mapping.h"
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
} // namespace

Vec3 PathTracer::TraceRay(const Scene& scene, const RayScene& rayScene, const Ray& ray, int depth, Random& rng) const
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

    // Self-emitted radiance (area lights / emissive surfaces). With no explicit
    // light sampling, emissive geometry hit by bounce rays is what lights the
    // scene (brute-force GI), so a closed room is lit purely by its ceiling
    // panel — the Cornell-box model.
    const Vec3 emitted = material.emission;
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
    return emitted + attenuation * TraceRay(scene, rayScene, scattered, depth - 1, rng);
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
                    color = color + TraceRay(scene, rayScene, ray, maxDepth, rng);
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
