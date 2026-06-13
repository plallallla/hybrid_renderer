#pragma once

#include "../../core/random.h"
#include "../../geometry/ray_scene.h"
#include "../irenderer.h"

namespace hr
{
// CPU path tracer (--mode pathtrace). It consumes the same shared Scene /
// Camera / PBRLite Material as the rasterizer, builds a RayScene from the
// flattened TracePrimitive list (geometry.md / render.md §20) and interprets
// the PBRLite material as a simple BSDF (material.md §7.2 / render.md §15):
//   metallic <  0.5 -> diffuse (Lambertian) bounce
//   metallic >= 0.5 -> rough-metal reflection bounce
// Radiance comes from rays that escape the scene and sample Scene::background
// (a constant sky), so the test scene should use a non-black background.
class PathTracer final : public IRenderer
{
public:
    void Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings) override;

private:
    Vec3 TraceRay(const Scene& scene, const RayScene& rayScene, const Ray& ray, int depth, Random& rng) const;
};
} // namespace hr
