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
} // namespace hr
