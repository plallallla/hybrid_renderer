// Temporary comparison driver: renders one scene with the pure rasterizer, the
// Hybrid renderer, and the brute-force PathTracer, then writes a 3-up montage.
// Not part of the shipped app; the CMake `compare` target is scaffolding and is
// reverted after use.
#include "../src/asset/scene_loader.h"
#include "../src/core/framebuffer.h"
#include "../src/core/png_writer.h"
#include "../src/core/render_settings.h"
#include "../src/renderer/hybrid/hybrid_renderer.h"
#include "../src/renderer/path_tracer/path_tracer.h"
#include "../src/renderer/rasterizer/rasterizer.h"

#include <chrono>
#include <iostream>
#include <string>

namespace
{
hr::Scene Load(const std::string& rel)
{
    hr::Scene scene;
    for (const std::string& prefix : {"", "../", "../../"})
    {
        if (hr::LoadSceneFromJson(prefix + rel, scene))
        {
            return scene;
        }
    }
    std::cerr << "Failed to load " << rel << "\n";
    return hr::CreateDefaultScene();
}

template <typename R>
double TimedRender(R& renderer, const hr::Scene& scene, hr::Framebuffer& fb, const hr::RenderSettings& s)
{
    const auto t0 = std::chrono::steady_clock::now();
    renderer.Render(scene, scene.camera, fb, s);
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}
} // namespace

int main(int argc, char** argv)
{
    const std::string scenePath = argc > 1 ? argv[1] : "assets/scenes/indoor_room.json";
    const std::string outPath = argc > 2 ? argv[2] : "compare.png";
    const int width = 640;
    const int height = 480;

    hr::Scene scene = Load(scenePath);

    // Pure rasterization: direct area lighting only, no shadows/AO/reflection/GI.
    hr::RenderSettings rs;
    rs.width = width;
    rs.height = height;

    // Hybrid: raster GBuffer + ray-traced shadows / AO / reflection.
    hr::RenderSettings hs;
    hs.width = width;
    hs.height = height;
    hs.enableRayTracedShadow = true;
    hs.enableRayTracedAO = true;
    hs.enableRayTracedReflection = true;
    hs.areaLightSamples = 128;
    hs.aoSamples = 32;
    hs.aoRadius = 2.0f;
    hs.useBVH = true;

    // PathTracer: brute-force GI, lit purely by the emissive ceiling panel.
    hr::RenderSettings ps;
    ps.width = width;
    ps.height = height;
    ps.samplesPerPixel = 1024;
    ps.maxDepth = 16;
    ps.exposure = 1.0f;
    ps.enableGammaCorrection = true;
    ps.useBVH = true;

    hr::SoftwareRasterizer raster;
    hr::HybridRenderer hybrid;
    hr::PathTracer pathTracer;
    hr::Framebuffer fbR, fbH, fbP;

    const double tR = TimedRender(raster, scene, fbR, rs);
    std::cout << "Raster:    " << tR << " s\n";
    const double tH = TimedRender(hybrid, scene, fbH, hs);
    std::cout << "Hybrid:    " << tH << " s\n";
    const double tP = TimedRender(pathTracer, scene, fbP, ps);
    std::cout << "PathTrace: " << tP << " s (" << ps.samplesPerPixel << " spp, depth " << ps.maxDepth << ")\n";

    // 3-up montage with thin separators.
    const int gap = 8;
    const hr::Vec3 sep{0.05f, 0.05f, 0.05f};
    hr::Image<hr::Vec3> montage(width * 3 + gap * 2, height, sep);
    const hr::Framebuffer* panels[3] = {&fbR, &fbH, &fbP};
    for (int p = 0; p < 3; ++p)
    {
        const int x0 = p * (width + gap);
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                montage.At(x0 + x, y) = panels[p]->color.At(x, y);
            }
        }
    }

    if (!hr::WritePng(outPath, montage))
    {
        std::cerr << "Failed to write " << outPath << "\n";
        return 1;
    }
    std::cout << "Wrote " << outPath << " (left=Raster, mid=Hybrid, right=PathTrace)\n";
    return 0;
}
