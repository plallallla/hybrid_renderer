#pragma once

#include "../core/math.h"

namespace hr
{
struct DirectionalLight
{
    Vec3 direction{-1.0f, -1.0f, -1.0f};
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};

// Positional point light. Like the reference (hellodx12/kill_cow) shader, the
// per-pixel light direction is L = normalize(position - worldPos) and the
// radiance is color * intensity with no distance attenuation, so it behaves
// like the lightPos/lightColor pair in that PBR pixel shader.
struct PointLight
{
    Vec3 position{5.0f, 5.0f, -5.0f};
    Vec3 color{5.0f, 5.0f, 5.0f};
    float intensity = 1.0f;
};

// Rectangular area light. The rectangle is centered at `center` and spanned by
// the two half-edge vectors `uVec` / `vVec` (so its corners are center ± uVec ±
// vVec). Emitted radiance is color * intensity. The loader also materializes an
// emissive quad mesh for each area light so it is both hittable by the path
// tracer (brute-force GI) and visible in the raster/hybrid GBuffer; the Hybrid
// renderer additionally samples this struct directly for soft shadows.
struct AreaLight
{
    Vec3 center{0.0f, 4.0f, 0.0f};
    Vec3 uVec{1.0f, 0.0f, 0.0f};
    Vec3 vVec{0.0f, 0.0f, 1.0f};
    Vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;

    Vec3 Radiance() const { return color * intensity; }
    Vec3 Normal() const { return Normalize(Cross(uVec, vVec)); }
    // Full-rectangle area (uVec / vVec are half-edges, so each full edge is 2x).
    float Area() const { return 4.0f * Length(Cross(uVec, vVec)); }
};
} // namespace hr
