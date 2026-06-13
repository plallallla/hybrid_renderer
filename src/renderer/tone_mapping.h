#pragma once

#include "../core/math.h"

#include <cmath>

namespace hr
{
inline Vec3 ApplyExposure(const Vec3& color, float exposure)
{
    return color * exposure;
}

inline Vec3 GammaCorrect(const Vec3& color)
{
    return {
        std::pow(Clamp(color.x, 0.0f, 1.0f), 1.0f / 2.2f),
        std::pow(Clamp(color.y, 0.0f, 1.0f), 1.0f / 2.2f),
        std::pow(Clamp(color.z, 0.0f, 1.0f), 1.0f / 2.2f),
    };
}
} // namespace hr
