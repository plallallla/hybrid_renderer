#pragma once

#include "../irenderer.h"

namespace hr
{
// Hybrid renderer. Primary visibility comes from the software
// rasterizer's PBR GBuffer; secondary effects come from ray queries against a
// shared RayScene:
//   V1  ray-traced hard shadows   (settings.enableRayTracedShadow)
//   V2  ray-traced ambient occ.   (settings.enableRayTracedAO)
//   V3  ray-traced reflection     (settings.enableRayTracedReflection)
// It does not re-implement rasterization; it calls
// SoftwareRasterizer::RenderGBuffer and composes lighting on top.
class HybridRenderer final : public IRenderer
{
public:
    void Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings) override;
};
} // namespace hr
