#pragma once

#include "../core/framebuffer.h"
#include "render_settings.h"
#include "../scene/camera.h"
#include "../scene/scene.h"

namespace hr
{
class IRenderer
{
public:
    virtual ~IRenderer() = default;

    virtual void Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings) = 0;
};
} // namespace hr
