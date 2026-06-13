#pragma once

#include "bvh.h"
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

    // Toggle the acceleration structure off to fall back to brute-force
    // traversal. Used for BVH-vs-brute-force correctness comparison; the public
    // Intersect/Occluded results must be identical either way.
    void SetUseBVH(bool useBVH) { m_useBVH = useBVH; }
    bool UsesBVH() const { return m_useBVH && !m_bvh.Empty(); }

private:
    std::vector<TracePrimitive> m_primitives;
    BVH m_bvh;
    bool m_useBVH = true;
};
} // namespace hr
