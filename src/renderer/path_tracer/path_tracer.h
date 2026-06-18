#pragma once

#include "../../core/random.h"
#include "../../geometry/ray_scene.h"
#include "../irenderer.h"

namespace hr
{
// CPU path tracer. It consumes the shared Scene / Camera / Material data,
// builds a RayScene from TracePrimitives, and interprets the material as a
// small BSDF:
//   metallic <  0.5 -> diffuse (Lambertian) bounce
//   metallic >= 0.5 -> rough-metal reflection bounce
// Radiance comes from Scene::background, emissive geometry and explicit
// area-light sampling.
class PathTracer final : public IRenderer
{
public:
    void Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings) override;

private:
    Vec3 TraceRay(
        const Scene& scene,
        const RayScene& rayScene,
        const Ray& ray,
        int depth,
        Random& rng,
        const RenderSettings& settings) const;
};
} // namespace hr
