#include "../src/core/framebuffer.h"
#include "../src/renderer/gbuffer.h"
#include "../src/renderer/rasterizer/rasterizer.h"
#include "../src/renderer/render_settings.h"
#include "../src/scene/scene.h"

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

Scene MakeSingleTriangleScene()
{
    Scene scene;
    scene.background = {0.0f, 0.0f, 0.0f};
    scene.camera.position = {0.0f, 0.0f, 3.0f};
    scene.camera.target = {0.0f, 0.0f, 0.0f};
    scene.camera.up = {0.0f, 1.0f, 0.0f};

    Material material;
    material.baseColor = {0.2f, 0.4f, 0.6f};
    material.metallic = 0.8f;
    material.roughness = 0.3f;
    scene.materials.push_back(material);

    scene.lights.push_back(DirectionalLight{});

    Mesh mesh;
    mesh.materialId = 0;
    mesh.vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
    };
    mesh.triangles.push_back({0, 1, 2});
    scene.meshes.push_back(mesh);
    return scene;
}
} // namespace

int main()
{
    const Scene scene = MakeSingleTriangleScene();

    RenderSettings settings;
    settings.width = 64;
    settings.height = 64;

    SoftwareRasterizer rasterizer;
    GBuffer gbuffer;
    rasterizer.RenderGBuffer(scene, scene.camera, gbuffer, settings);

    Check(gbuffer.valid.Width() == 64 && gbuffer.valid.Height() == 64, "RenderGBuffer resizes GBuffer to settings size");

    // Center pixel lies inside the triangle (which spans the origin).
    const int cx = settings.width / 2;
    const int cy = settings.height / 2;
    const bool centerValid = gbuffer.valid.Data()[static_cast<std::size_t>(cy) * settings.width + cx];
    Check(centerValid, "GBuffer.valid is true on a covered pixel");
    Check(gbuffer.materialID.At(cx, cy) == 0, "GBuffer.materialID matches mesh material on covered pixel");

    const Vec3 baseColor = gbuffer.baseColor.At(cx, cy);
    Check(NearEqual(baseColor.x, 0.2f) && NearEqual(baseColor.y, 0.4f) && NearEqual(baseColor.z, 0.6f),
        "GBuffer.baseColor matches material baseColor (untextured)");
    Check(NearEqual(gbuffer.metallic.At(cx, cy), 0.8f), "GBuffer.metallic matches material");
    Check(NearEqual(gbuffer.roughness.At(cx, cy), 0.3f), "GBuffer.roughness matches material");

    const Vec3 normal = gbuffer.normal.At(cx, cy);
    Check(NearEqual(normal.z, 1.0f, 1e-2f), "GBuffer.normal faces the camera (+z)");

    const Vec3 position = gbuffer.position.At(cx, cy);
    Check(NearEqual(position.z, 0.0f, 1e-2f), "GBuffer.position lies on the triangle plane (z=0)");

    // A corner pixel is background (outside the triangle).
    const bool cornerValid = gbuffer.valid.Data()[0];
    Check(!cornerValid, "GBuffer.valid is false on an uncovered (background) pixel");
    Check(gbuffer.materialID.At(0, 0) == -1, "GBuffer.materialID stays -1 on background pixel");

    if (g_failures == 0)
    {
        std::printf("\nAll GBuffer tests passed.\n");
        return 0;
    }

    std::printf("\n%d GBuffer test(s) failed.\n", g_failures);
    return 1;
}
