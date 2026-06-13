#include "bvh.h"

#include <algorithm>

namespace hr
{
void BVH::Build(std::vector<TracePrimitive>& primitives)
{
    m_nodes.clear();
    m_primitives = &primitives;

    if (primitives.empty())
    {
        return;
    }

    m_nodes.reserve(primitives.size() * 2);
    BuildRecursive(primitives, 0, static_cast<int>(primitives.size()));
}

int BVH::BuildRecursive(std::vector<TracePrimitive>& primitives, int start, int count)
{
    BVHNode node;
    for (int i = start; i < start + count; ++i)
    {
        node.bounds.Expand(primitives[static_cast<size_t>(i)].Bounds());
    }

    const int nodeIndex = static_cast<int>(m_nodes.size());
    m_nodes.push_back(node);

    if (count <= LeafSize)
    {
        m_nodes[static_cast<size_t>(nodeIndex)].first = start;
        m_nodes[static_cast<size_t>(nodeIndex)].count = count;
        return nodeIndex;
    }

    AABB centroidBounds;
    for (int i = start; i < start + count; ++i)
    {
        centroidBounds.Expand(primitives[static_cast<size_t>(i)].Center());
    }
    const int axis = centroidBounds.LongestAxis();
    const int mid = start + count / 2;

    std::nth_element(
        primitives.begin() + start,
        primitives.begin() + mid,
        primitives.begin() + start + count,
        [axis](const TracePrimitive& a, const TracePrimitive& b) {
            return a.Center()[axis] < b.Center()[axis];
        });

    const int left = BuildRecursive(primitives, start, mid - start);
    const int right = BuildRecursive(primitives, mid, start + count - mid);

    m_nodes[static_cast<size_t>(nodeIndex)].left = left;
    m_nodes[static_cast<size_t>(nodeIndex)].right = right;
    return nodeIndex;
}

bool BVH::Intersect(const Ray& ray, HitRecord& hit, float tMin, float tMax) const
{
    if (m_nodes.empty() || m_primitives == nullptr)
    {
        return false;
    }

    bool hitAnything = false;
    float closest = tMax;

    int stack[64];
    int stackSize = 0;
    stack[stackSize++] = 0;

    while (stackSize > 0)
    {
        const BVHNode& node = m_nodes[static_cast<size_t>(stack[--stackSize])];
        if (!node.bounds.Intersect(ray, tMin, closest))
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (int i = 0; i < node.count; ++i)
            {
                HitRecord tempHit;
                if ((*m_primitives)[static_cast<size_t>(node.first + i)].Intersect(ray, tMin, closest, tempHit))
                {
                    hitAnything = true;
                    closest = tempHit.t;
                    hit = tempHit;
                }
            }
        }
        else
        {
            stack[stackSize++] = node.left;
            stack[stackSize++] = node.right;
        }
    }

    return hitAnything;
}

bool BVH::Occluded(const Ray& ray, float tMin, float tMax) const
{
    if (m_nodes.empty() || m_primitives == nullptr)
    {
        return false;
    }

    int stack[64];
    int stackSize = 0;
    stack[stackSize++] = 0;

    while (stackSize > 0)
    {
        const BVHNode& node = m_nodes[static_cast<size_t>(stack[--stackSize])];
        if (!node.bounds.Intersect(ray, tMin, tMax))
        {
            continue;
        }

        if (node.IsLeaf())
        {
            for (int i = 0; i < node.count; ++i)
            {
                HitRecord tempHit;
                if ((*m_primitives)[static_cast<size_t>(node.first + i)].Intersect(ray, tMin, tMax, tempHit))
                {
                    return true;
                }
            }
        }
        else
        {
            stack[stackSize++] = node.left;
            stack[stackSize++] = node.right;
        }
    }

    return false;
}
} // namespace hr
