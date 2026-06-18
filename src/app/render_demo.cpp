#include "render_demo.h"

#include "../core/png_writer.h"
#include "../renderer/hybrid/hybrid_renderer.h"
#include "../renderer/irenderer.h"
#include "../renderer/path_tracer/path_tracer.h"
#include "../renderer/rasterizer/rasterizer.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <utility>

namespace hr
{
namespace
{
std::string ReadString(const JsonValue* value, const std::string& fallback)
{
    return value && value->IsString() ? value->AsString() : fallback;
}

bool ReadBool(const JsonValue* value, bool fallback)
{
    return value && value->IsBool() ? value->AsBool(fallback) : fallback;
}

int ReadInt(const JsonValue* value, int fallback)
{
    return value && value->IsNumber() ? static_cast<int>(value->AsNumber(fallback)) : fallback;
}

float ReadFloat(const JsonValue* value, float fallback)
{
    return value && value->IsNumber() ? static_cast<float>(value->AsNumber(fallback)) : fallback;
}

RenderMode ParseMode(const std::string& mode)
{
    if (mode == "raster")
    {
        return RenderMode::Raster;
    }
    if (mode == "pathtrace")
    {
        return RenderMode::PathTrace;
    }
    return RenderMode::Hybrid;
}

std::string ModeName(RenderMode mode)
{
    switch (mode)
    {
        case RenderMode::Raster: return "raster";
        case RenderMode::PathTrace: return "pathtrace";
        case RenderMode::Hybrid: return "hybrid";
    }
    return "hybrid";
}

RenderJob BuildDefaultJob()
{
    RenderJob job;
    job.name = "Hybrid";
    job.mode = RenderMode::Hybrid;
    job.outputPath = "output_hybrid.png";
    job.settings.width = 600;
    job.settings.height = 600;
    job.settings.enableRayTracedShadow = true;
    job.settings.enableRayTracedAO = true;
    job.settings.enableRayTracedReflection = true;
    job.settings.aoSamples = 24;
    job.settings.aoRadius = 1.0f;
    job.settings.areaLightSamples = 64;
    job.settings.useBVH = true;
    return job;
}

RenderJob BuildJob(const JsonValue& value)
{
    RenderJob job = BuildDefaultJob();
    if (!value.IsObject())
    {
        return job;
    }

    job.name = ReadString(value.Find("name"), job.name);
    job.mode = ParseMode(ReadString(value.Find("mode"), ModeName(job.mode)));
    job.outputPath = ReadString(value.Find("output"), job.outputPath);

    job.settings.width = ReadInt(value.Find("width"), job.settings.width);
    job.settings.height = ReadInt(value.Find("height"), job.settings.height);
    job.settings.samplesPerPixel = ReadInt(value.Find("spp"), job.settings.samplesPerPixel);
    job.settings.maxDepth = ReadInt(value.Find("maxDepth"), job.settings.maxDepth);
    job.settings.enableRayTracedShadow = ReadBool(value.Find("shadow"), job.settings.enableRayTracedShadow);
    job.settings.enableRayTracedAO = ReadBool(value.Find("ao"), job.settings.enableRayTracedAO);
    job.settings.enableRayTracedReflection = ReadBool(value.Find("reflection"), job.settings.enableRayTracedReflection);
    job.settings.aoSamples = ReadInt(value.Find("aoSamples"), job.settings.aoSamples);
    job.settings.aoRadius = ReadFloat(value.Find("aoRadius"), job.settings.aoRadius);
    job.settings.areaLightSamples = ReadInt(value.Find("areaLightSamples"), job.settings.areaLightSamples);
    job.settings.exposure = ReadFloat(value.Find("exposure"), job.settings.exposure);
    job.settings.enableGammaCorrection = ReadBool(value.Find("gamma"), job.settings.enableGammaCorrection);
    job.settings.useBVH = ReadBool(value.Find("useBVH"), job.settings.useBVH);
    return job;
}

IRenderer& SelectRenderer(
    RenderMode mode,
    SoftwareRasterizer& rasterizer,
    HybridRenderer& hybridRenderer,
    PathTracer& pathTracer)
{
    switch (mode)
    {
        case RenderMode::Raster: return rasterizer;
        case RenderMode::PathTrace: return pathTracer;
        case RenderMode::Hybrid: return hybridRenderer;
    }
    return hybridRenderer;
}

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
} // namespace

DemoConfig BuildDemoConfig(const JsonValue& root)
{
    DemoConfig config;
    config.jobs.push_back(BuildDefaultJob());

    const JsonValue* demo = root.Find("renderDemo");
    if (!demo || !demo->IsObject())
    {
        return config;
    }

    config.openOutputs = ReadBool(demo->Find("openOutputs"), config.openOutputs);
    config.montageOutput = ReadString(demo->Find("montage"), config.montageOutput);

    if (const JsonValue* jobs = demo->Find("jobs"))
    {
        if (jobs->IsArray() && !jobs->AsArray().empty())
        {
            config.jobs.clear();
            for (const JsonValue& jobValue : jobs->AsArray())
            {
                config.jobs.push_back(BuildJob(jobValue));
            }
        }
    }

    return config;
}

bool WriteMontage(const std::string& path, const std::vector<Framebuffer>& frames, int gap)
{
    if (path.empty() || frames.empty())
    {
        return true;
    }

    const int height = frames[0].color.Height();
    int width = 0;
    for (const Framebuffer& frame : frames)
    {
        if (frame.color.Empty() || frame.color.Height() != height)
        {
            return false;
        }
        width += frame.color.Width();
    }
    width += gap * static_cast<int>(frames.size() - 1);

    Image<Vec3> montage(width, height, Vec3{0.04f, 0.04f, 0.04f});
    int offsetX = 0;
    for (const Framebuffer& frame : frames)
    {
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < frame.color.Width(); ++x)
            {
                montage.At(offsetX + x, y) = frame.color.At(x, y);
            }
        }
        offsetX += frame.color.Width() + gap;
    }

    return WritePng(path, montage);
}

int RunRenderDemo(const Scene& scene, const DemoConfig& config)
{
    SoftwareRasterizer rasterizer;
    HybridRenderer hybridRenderer;
    PathTracer pathTracer;
    std::vector<Framebuffer> frames;
    frames.reserve(config.jobs.size());

    for (const RenderJob& job : config.jobs)
    {
        Framebuffer framebuffer;
        IRenderer& renderer = SelectRenderer(job.mode, rasterizer, hybridRenderer, pathTracer);

        const auto start = std::chrono::steady_clock::now();
        renderer.Render(scene, scene.camera, framebuffer, job.settings);
        const auto end = std::chrono::steady_clock::now();
        const double seconds = std::chrono::duration<double>(end - start).count();

        if (!WritePng(job.outputPath, framebuffer.color))
        {
            std::cerr << "Failed to write " << job.outputPath << std::endl;
            return 1;
        }

        std::cout << "Rendered " << job.name << " [" << ModeName(job.mode) << "] -> "
                  << job.outputPath << " in " << seconds << " s" << std::endl;

        if (config.openOutputs)
        {
            OpenImage(job.outputPath);
        }
        frames.push_back(std::move(framebuffer));
    }

    if (!config.montageOutput.empty())
    {
        if (!WriteMontage(config.montageOutput, frames))
        {
            std::cerr << "Failed to write montage " << config.montageOutput << std::endl;
            return 1;
        }
        std::cout << "Wrote montage " << config.montageOutput << std::endl;
        if (config.openOutputs)
        {
            OpenImage(config.montageOutput);
        }
    }

    return 0;
}
} // namespace hr
