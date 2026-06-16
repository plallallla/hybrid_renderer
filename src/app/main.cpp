#include "../asset/scene_loader.h"
#include "../core/framebuffer.h"
#include "../core/png_writer.h"
#include "../core/render_settings.h"
#include "../renderer/hybrid/hybrid_renderer.h"
#include "../renderer/path_tracer/path_tracer.h"
#include "../renderer/rasterizer/rasterizer.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
// Opens the rendered image with the OS default viewer. `open` is macOS-only,
// so dispatch per-platform (Windows: start, Linux: xdg-open) to actually work.
void OpenImage(const std::string& path)
{
#if defined(_WIN32)
    const std::string command = "start \"\" \"" + path + "\"";
#elif defined(__APPLE__)
    const std::string command = "open \"" + path + "\"";
#else
    const std::string command = "xdg-open \"" + path + "\"";
#endif
    std::system(command.c_str());
}

// ---------------------------------------------------------------------------
// Render configuration. Edit these values directly instead of passing CLI
// arguments. (CLI parsing was intentionally removed.)
// ---------------------------------------------------------------------------
struct Options
{
    // "raster" | "pathtrace" | "hybrid"
    // Cornell Box is set up for both "hybrid" and "pathtrace"; flip this to
    // switch which renderer runs.
    std::string mode = "hybrid";

    // Scene JSON to load. If empty, falls back to phase3_demo.json.
    std::string scenePath = "assets/scenes/cornell_box.json";

    // Output image path.
    std::string outputPath = "output.png";

    // Square output to match the canonical Cornell Box framing.
    int width = 600;
    int height = 600;

    // PathTracer controls (ignored by raster / hybrid). Cornell Box is closed
    // and lit only by the ceiling panel, so it needs both plenty of samples
    // and enough bounce depth for indirect light to converge.
    int spp = 256;      // samples per pixel
    int maxDepth = 8;   // max ray bounces

    // Hybrid feature toggles (ignored by raster / pathtrace).
    bool enableShadow = true;
    bool enableAO = true;
    bool enableReflection = true;

    // Ray-traversal acceleration (pathtrace / hybrid). false = brute force.
    bool useBVH = true;
};
} // namespace

int main()
{
    const Options options;

    if (options.mode != "raster" && options.mode != "pathtrace" && options.mode != "hybrid")
    {
        std::cerr << "Unsupported mode '" << options.mode << "'. Use raster, pathtrace or hybrid." << std::endl;
        return 1;
    }

    // Try the scene path relative to a few working directories (repo root or
    // the build/Debug output dir) so the hardcoded relative path resolves
    // regardless of where the executable is launched from.
    const std::string sceneRelative = options.scenePath.empty() ? "assets/scenes/phase3_demo.json" : options.scenePath;
    hr::Scene scene;
    bool sceneLoaded = false;
    for (const std::string& prefix : {"", "../", "../../"})
    {
        if (hr::LoadSceneFromJson(prefix + sceneRelative, scene))
        {
            sceneLoaded = true;
            break;
        }
    }
    if (!sceneLoaded)
    {
        std::cerr << "Failed to load scene '" << sceneRelative << "', using default scene." << std::endl;
        scene = hr::CreateDefaultScene();
    }

    hr::RenderSettings settings;
    settings.width = options.width;
    settings.height = options.height;
    settings.samplesPerPixel = options.spp;
    settings.maxDepth = options.maxDepth;
    settings.enableRayTracedShadow = options.enableShadow;
    settings.enableRayTracedAO = options.enableAO;
    settings.enableRayTracedReflection = options.enableReflection;
    settings.useBVH = options.useBVH;

    hr::Framebuffer framebuffer;
    hr::SoftwareRasterizer rasterizer;
    hr::PathTracer pathTracer;
    hr::HybridRenderer hybridRenderer;
    hr::IRenderer* renderer = &rasterizer;
    if (options.mode == "pathtrace")
    {
        renderer = &pathTracer;
    }
    else if (options.mode == "hybrid")
    {
        renderer = &hybridRenderer;
    }
    renderer->Render(scene, scene.camera, framebuffer, settings);

    if (!hr::WritePng(options.outputPath, framebuffer.color))
    {
        std::cerr << "Failed to write output image." << std::endl;
        return 1;
    }

    std::cout << "Rendered " << options.outputPath << " in " << options.mode << " mode." << std::endl;

    OpenImage(options.outputPath);
    return 0;
}
