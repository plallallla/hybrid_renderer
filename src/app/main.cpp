#include "../asset/scene_loader.h"
#include "../core/framebuffer.h"
#include "../core/png_writer.h"
#include "../core/render_settings.h"
#include "../renderer/rasterizer/rasterizer.h"

#include <cstdlib>
#include <exception>
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

struct CliOptions
{
    std::string mode = "raster";
    std::string scenePath;
    std::string outputPath = "output.png";
    int width = 800;
    int height = 600;
};

bool ParseArguments(int argc, char** argv, CliOptions& options)
{
    try
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            if (arg == "--mode" && i + 1 < argc)
            {
                options.mode = argv[++i];
            }
            else if (arg == "--scene" && i + 1 < argc)
            {
                options.scenePath = argv[++i];
            }
            else if (arg == "--output" && i + 1 < argc)
            {
                options.outputPath = argv[++i];
            }
            else if (arg == "--width" && i + 1 < argc)
            {
                options.width = std::stoi(argv[++i]);
            }
            else if (arg == "--height" && i + 1 < argc)
            {
                options.height = std::stoi(argv[++i]);
            }
            else if (arg == "--help")
            {
                return false;
            }
            else
            {
                std::cerr << "Unknown or incomplete argument: " << arg << '\n';
                return false;
            }
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Invalid command line argument: " << e.what() << '\n';
        return false;
    }
}

void PrintUsage()
{
    std::cout << "HybridCPU_Renderer\n"
              << "  --mode raster\n"
              << "  --scene scene.json\n"
              << "  --output output.png\n"
              << "  --width 800\n"
              << "  --height 600\n";
}
} // namespace

int main(int argc, char** argv)
{
    CliOptions options;
    if (!ParseArguments(argc, argv, options))
    {
        PrintUsage();
        return 1;
    }

    if (options.mode != "raster")
    {
        std::cerr << "Only --mode raster is implemented in this phase." << std::endl;
        return 1;
    }

    hr::Scene scene;
    if (!options.scenePath.empty())
    {
        if (!hr::LoadSceneFromJson(options.scenePath, scene))
        {
            std::cerr << "Failed to load scene from JSON, using default scene." << std::endl;
            scene = hr::CreateDefaultScene();
        }
    }
    else
    {
        if (!hr::LoadSceneFromJson("assets/scenes/phase3_demo.json", scene) &&
            !hr::LoadSceneFromJson("../assets/scenes/phase3_demo.json", scene))
        {
            scene = hr::CreateDefaultScene();
        }
    }

    hr::RenderSettings settings;
    settings.width = options.width;
    settings.height = options.height;

    hr::Framebuffer framebuffer;
    hr::SoftwareRasterizer renderer;
    renderer.Render(scene, scene.camera, framebuffer, settings);

    if (!hr::WritePng(options.outputPath, framebuffer.color))
    {
        std::cerr << "Failed to write output image." << std::endl;
        return 1;
    }

    std::cout << "Rendered " << options.outputPath << " in raster mode." << std::endl;

    OpenImage(options.outputPath);
    return 0;
}
