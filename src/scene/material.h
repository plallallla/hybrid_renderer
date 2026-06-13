#pragma once

#include "../core/math.h"

namespace hr
{
struct Material
{
    Vec3 baseColor{1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    int baseColorTexture = -1;
    int normalTexture = -1;
    int metallicRoughnessTexture = -1;
};
} // namespace hr
