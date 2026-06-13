#pragma once

#include "../gbuffer.h"
#include "../irenderer.h"

namespace hr
{
class SoftwareRasterizer final : public IRenderer
{
public:
    void Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings) override;

    // Rasterizes scene visibility and writes per-pixel geometry/material
    // attributes into the GBuffer (position/normal/baseColor/metallic/
    // roughness/depth/materialID/valid) without performing any lighting.
    // Hybrid rendering reuses this rather than re-implementing rasterization
    // (render.md §21).
    void RenderGBuffer(const Scene& scene, const Camera& camera, GBuffer& gbuffer, const RenderSettings& settings);
};
} // namespace hr
