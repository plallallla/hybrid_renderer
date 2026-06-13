#include "../src/core/math.h"
#include "../src/core/random.h"
#include "../src/geometry/ray_scene.h"
#include "../src/geometry/trace_primitive.h"
#include "../src/scene/camera.h"

#include <cmath>
#include <cstdio>

using namespace hr;

namespace
{
int g_failures = 0;

void Check(bool condition, const char* name)
{
    if (condition)
    {
        std::printf("[PASS] %s\n", name);
    }
    else
    {
        std::printf("[FAIL] %s\n", name);
        ++g_failures;
    }
}

bool NearEqual(float a, float b, float eps = 1e-3f)
{
    return std::fabs(a - b) < eps;
}

bool NearEqual(const Vec3& a, const Vec3& b, float eps = 1e-3f)
{
    return NearEqual(a.x, b.x, eps) && NearEqual(a.y, b.y, eps) && NearEqual(a.z, b.z, eps);
}
} // namespace

int main()
{
    // --- Task 2: core/math.h additions ---
    {
        Ray ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
        Check(NearEqual(ray.At(2.0f), {0.0f, 0.0f, -2.0f}), "Ray::At");
    }
    {
        const Vec3 incident{1.0f, -1.0f, 0.0f};
        const Vec3 normal{0.0f, 1.0f, 0.0f};
        Check(NearEqual(Reflect(incident, normal), {1.0f, 1.0f, 0.0f}), "Reflect");
    }
    {
        const Vec3 a{1.0f, 5.0f, -2.0f};
        const Vec3 b{3.0f, 2.0f, -1.0f};
        Check(NearEqual(Min(a, b), {1.0f, 2.0f, -2.0f}), "Min(Vec3,Vec3)");
        Check(NearEqual(Max(a, b), {3.0f, 5.0f, -1.0f}), "Max(Vec3,Vec3)");
    }
    {
        Vec3 v{1.0f, 2.0f, 3.0f};
        Check(NearEqual(v[0], 1.0f) && NearEqual(v[1], 2.0f) && NearEqual(v[2], 3.0f), "Vec3::operator[] const");
        v[1] = 5.0f;
        Check(NearEqual(v.y, 5.0f), "Vec3::operator[] non-const");
    }
    {
        Check(NearEqual(Radians(180.0f), PI), "Radians");
        Check(NearEqual(Lerp(0.0f, 10.0f, 0.5f), 5.0f), "Lerp(float,float,float)");
    }

    // --- Task 2: Camera::GenerateRay ---
    {
        Camera camera;
        camera.position = {0.0f, 0.0f, 3.0f};
        camera.target = {0.0f, 0.0f, 0.0f};
        camera.up = {0.0f, 1.0f, 0.0f};
        camera.fovYDegrees = 60.0f;
        camera.aspect = 1.0f;

        const Ray centerRay = camera.GenerateRay(0.5f, 0.5f);
        const Vec3 expectedForward = Normalize(camera.target - camera.position);
        Check(NearEqual(centerRay.direction, expectedForward), "Camera::GenerateRay center ray points at target");

        const Ray leftRay = camera.GenerateRay(0.0f, 0.5f);
        Check(leftRay.direction.x < centerRay.direction.x, "Camera::GenerateRay left edge bends toward -x");
    }

    // --- Task 2: core/random.h ---
    {
        Random rng(42);
        const float a = rng.NextFloat();
        const float b = rng.NextFloat();
        Check(a >= 0.0f && a <= 1.0f && b >= 0.0f && b <= 1.0f && a != b, "Random::NextFloat returns varying [0,1) samples");

        const Vec2 sample = rng.NextVec2();
        Check(sample.x >= 0.0f && sample.x <= 1.0f && sample.y >= 0.0f && sample.y <= 1.0f, "Random::NextVec2 in [0,1]^2");
    }

    // --- Task 3: TracePrimitive + RayScene (ground triangle + sphere) ---
    {
        TracePrimitive groundTri;
        groundTri.type = TracePrimitiveType::Triangle;
        groundTri.triangle.v0 = {{-10.0f, 0.0f, -10.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
        groundTri.triangle.v1 = {{10.0f, 0.0f, -10.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}};
        groundTri.triangle.v2 = {{0.0f, 0.0f, 10.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}};
        groundTri.materialID = 0;
        groundTri.primitiveID = 0;

        Sphere sphere;
        sphere.center = {0.0f, 1.0f, -3.0f};
        sphere.radius = 1.0f;
        sphere.materialId = 1;

        TracePrimitive spherePrim;
        spherePrim.type = TracePrimitiveType::Sphere;
        spherePrim.sphere = sphere;
        spherePrim.materialID = sphere.materialId;
        spherePrim.primitiveID = 1;

        // Triangle intersection in isolation.
        {
            const Ray downRay{{0.0f, 5.0f, -5.0f}, {0.0f, -1.0f, 0.0f}};
            HitRecord hit;
            const bool didHit = groundTri.Intersect(downRay, EPSILON, 1000.0f, hit);
            Check(didHit && NearEqual(hit.t, 5.0f) && NearEqual(hit.position, {0.0f, 0.0f, -5.0f}) && hit.materialID == 0,
                "TracePrimitive::Intersect triangle (Moller-Trumbore)");
        }

        // Sphere intersection in isolation.
        {
            const Ray sphereRay{{0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
            HitRecord hit;
            const bool didHit = spherePrim.Intersect(sphereRay, EPSILON, 1000.0f, hit);
            Check(didHit && NearEqual(hit.t, 7.0f) && hit.materialID == 1, "TracePrimitive::Intersect sphere");
        }

        RayScene rayScene;
        rayScene.Build({groundTri, spherePrim});
        Check(rayScene.IsValid(), "RayScene::Build populates primitives");

        // Ray straight down hits the ground plane.
        {
            const Ray downRay{{0.0f, 5.0f, -5.0f}, {0.0f, -1.0f, 0.0f}};
            HitRecord hit;
            const bool didHit = rayScene.Intersect(downRay, hit);
            Check(didHit && NearEqual(hit.t, 5.0f) && hit.materialID == 0, "RayScene::Intersect ground triangle");
        }

        // Ray toward the sphere hits it (closer than the ground plane it is parallel to).
        {
            const Ray sphereRay{{0.0f, 1.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
            HitRecord hit;
            const bool didHit = rayScene.Intersect(sphereRay, hit);
            Check(didHit && hit.materialID == 1 && NearEqual(hit.t, 7.0f), "RayScene::Intersect picks closest hit (sphere)");
        }

        // Occlusion: a ray above the sphere pointing down is blocked before tMax.
        {
            const Ray occludeRay{{0.0f, 5.0f, -3.0f}, {0.0f, -1.0f, 0.0f}};
            Check(rayScene.Occluded(occludeRay, EPSILON, 100.0f), "RayScene::Occluded detects blocking geometry");
        }

        // A ray far away from both primitives should not be occluded.
        {
            const Ray missRay{{100.0f, 100.0f, 100.0f}, {1.0f, 0.0f, 0.0f}};
            Check(!rayScene.Occluded(missRay, EPSILON, 100.0f), "RayScene::Occluded returns false for a clear ray");
        }
    }

    // --- Task 7: BVH vs brute-force consistency ---
    {
        Random rng(1234);
        std::vector<TracePrimitive> primitives;

        // A field of spheres.
        for (int i = 0; i < 150; ++i)
        {
            Sphere sphere;
            sphere.center = {
                (rng.NextFloat() - 0.5f) * 20.0f,
                (rng.NextFloat() - 0.5f) * 20.0f,
                (rng.NextFloat() - 0.5f) * 20.0f,
            };
            sphere.radius = 0.2f + rng.NextFloat() * 0.8f;
            sphere.materialId = i % 4;

            TracePrimitive prim;
            prim.type = TracePrimitiveType::Sphere;
            prim.sphere = sphere;
            prim.materialID = sphere.materialId;
            prim.primitiveID = static_cast<int>(primitives.size());
            primitives.push_back(prim);
        }
        // Some triangles mixed in.
        for (int i = 0; i < 60; ++i)
        {
            const Vec3 base{
                (rng.NextFloat() - 0.5f) * 20.0f,
                (rng.NextFloat() - 0.5f) * 20.0f,
                (rng.NextFloat() - 0.5f) * 20.0f,
            };
            TracePrimitive prim;
            prim.type = TracePrimitiveType::Triangle;
            prim.triangle.v0 = {base, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}};
            prim.triangle.v1 = {base + Vec3{rng.NextFloat() * 2.0f, rng.NextFloat() * 2.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}};
            prim.triangle.v2 = {base + Vec3{0.0f, rng.NextFloat() * 2.0f, rng.NextFloat() * 2.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 1.0f}};
            prim.materialID = i % 4;
            prim.primitiveID = static_cast<int>(primitives.size());
            primitives.push_back(prim);
        }

        RayScene scene;
        scene.Build(primitives);
        Check(scene.UsesBVH(), "RayScene uses BVH after Build");

        int intersectMismatches = 0;
        int occludedMismatches = 0;
        const int rayCount = 2000;
        for (int i = 0; i < rayCount; ++i)
        {
            const Vec3 origin{
                (rng.NextFloat() - 0.5f) * 30.0f,
                (rng.NextFloat() - 0.5f) * 30.0f,
                (rng.NextFloat() - 0.5f) * 30.0f,
            };
            const Vec3 dir = Normalize({
                rng.NextFloat() * 2.0f - 1.0f,
                rng.NextFloat() * 2.0f - 1.0f,
                rng.NextFloat() * 2.0f - 1.0f,
            });
            const Ray ray{origin, dir};
            const float tMax = 50.0f;

            scene.SetUseBVH(true);
            HitRecord bvhHit;
            const bool bvhDidHit = scene.Intersect(ray, bvhHit, EPSILON, tMax);
            const bool bvhOccluded = scene.Occluded(ray, EPSILON, tMax);

            scene.SetUseBVH(false);
            HitRecord bruteHit;
            const bool bruteDidHit = scene.Intersect(ray, bruteHit, EPSILON, tMax);
            const bool bruteOccluded = scene.Occluded(ray, EPSILON, tMax);

            if (bvhDidHit != bruteDidHit ||
                (bvhDidHit && (bvhHit.primitiveID != bruteHit.primitiveID || !NearEqual(bvhHit.t, bruteHit.t))))
            {
                ++intersectMismatches;
            }
            if (bvhOccluded != bruteOccluded)
            {
                ++occludedMismatches;
            }
        }
        scene.SetUseBVH(true);

        Check(intersectMismatches == 0, "BVH vs brute-force Intersect identical over 2000 random rays");
        Check(occludedMismatches == 0, "BVH vs brute-force Occluded identical over 2000 random rays");
    }

    if (g_failures == 0)
    {
        std::printf("\nAll geometry tests passed.\n");
        return 0;
    }

    std::printf("\n%d geometry test(s) failed.\n", g_failures);
    return 1;
}
