#pragma once

#include "../asset/json.h"
#include "../core/framebuffer.h"
#include "../renderer/render_mode.h"
#include "../renderer/render_settings.h"
#include "../scene/scene.h"

#include <string>
#include <vector>

namespace hr
{
struct RenderJob
{
    std::string name = "Hybrid";
    RenderMode mode = RenderMode::Hybrid;
    std::string outputPath = "output.png";
    RenderSettings settings;
};

struct DemoConfig
{
    bool openOutputs = false;
    std::string montageOutput;
    std::vector<RenderJob> jobs;
};

DemoConfig BuildDemoConfig(const JsonValue& root);
bool WriteMontage(const std::string& path, const std::vector<Framebuffer>& frames, int gap = 8);
int RunRenderDemo(const Scene& scene, const DemoConfig& config);
} // namespace hr
