#pragma once

#include "trace_primitive.h"

#include <vector>

namespace hr
{
struct BVHNode
{
    AABB bounds;
    int left = -1;
    int right = -1;
    int first = 0;
    int count = 0;

    bool IsLeaf() const { return count > 0; }
};

// Median-split BVH over a TracePrimitive list (geometry.md §10-12). Build()
// reorders the caller's primitive vector in place (nth_element on the longest
// centroid axis) and keeps a pointer to it; leaf nodes reference contiguous
// [first, first+count) ranges. Leaf size is 4. Traversal is an explicit stack,
// no recursion, identical closest-hit / occlusion semantics to brute force.
class BVH
{
public:
    void Build(std::vector<TracePrimitive>& primitives);

    bool Intersect(const Ray& ray, HitRecord& hit, float tMin, float tMax) const;
    bool Occluded(const Ray& ray, float tMin, float tMax) const;

    bool Empty() const { return m_nodes.empty(); }

private:
    int BuildRecursive(std::vector<TracePrimitive>& primitives, int start, int count);

    std::vector<BVHNode> m_nodes;
    const std::vector<TracePrimitive>* m_primitives = nullptr;

    static constexpr int LeafSize = 4;
};
} // namespace hr
