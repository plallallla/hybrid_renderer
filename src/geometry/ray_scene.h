#pragma once

#include "trace_primitive.h"

#include <limits>
#include <vector>

namespace hr
{
class RayScene
{
public:
    void Build(const std::vector<TracePrimitive>& primitives);

    bool Intersect(
        const Ray& ray,
        HitRecord& hit,
        float tMin = EPSILON,
        float tMax = std::numeric_limits<float>::max()) const;

    bool Occluded(const Ray& ray, float tMin, float tMax) const;

    bool IsValid() const { return !m_primitives.empty(); }

private:
    std::vector<TracePrimitive> m_primitives;
};
} // namespace hr
