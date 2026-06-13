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
} // namespace hr
