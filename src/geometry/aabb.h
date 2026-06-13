#pragma once

#include "ray.h"

#include <algorithm>
#include <limits>

namespace hr
{
struct AABB
{
    Vec3 min{
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::max(),
    };
    Vec3 max{
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
        -std::numeric_limits<float>::max(),
    };

    AABB() = default;
    AABB(const Vec3& inMin, const Vec3& inMax)
        : min(inMin)
        , max(inMax)
    {
    }

    Vec3 Center() const { return (min + max) * 0.5f; }
    Vec3 Extent() const { return max - min; }

    void Expand(const Vec3& p)
    {
        min = Min(min, p);
        max = Max(max, p);
    }

    void Expand(const AABB& box)
    {
        min = Min(min, box.min);
        max = Max(max, box.max);
    }

    int LongestAxis() const
    {
        const Vec3 e = Extent();
        if (e.x > e.y && e.x > e.z)
        {
            return 0;
        }
        if (e.y > e.z)
        {
            return 1;
        }
        return 2;
    }

    bool Intersect(const Ray& ray, float tMin, float tMax) const
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            const float invD = 1.0f / ray.direction[axis];
            float t0 = (min[axis] - ray.origin[axis]) * invD;
            float t1 = (max[axis] - ray.origin[axis]) * invD;
            if (invD < 0.0f)
            {
                std::swap(t0, t1);
            }

            tMin = std::max(tMin, t0);
            tMax = std::min(tMax, t1);
            if (tMax <= tMin)
            {
                return false;
            }
        }
        return true;
    }
};
} // namespace hr
