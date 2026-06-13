#pragma once

#include "aabb.h"

namespace hr
{
struct Sphere
{
    Vec3 center{0.0f, 0.0f, 0.0f};
    float radius = 1.0f;
    int materialId = 0;

    AABB Bounds() const
    {
        const Vec3 r{radius, radius, radius};
        return AABB(center - r, center + r);
    }
};
} // namespace hr
