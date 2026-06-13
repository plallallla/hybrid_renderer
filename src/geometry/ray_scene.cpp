#include "ray_scene.h"

namespace hr
{
void RayScene::Build(const std::vector<TracePrimitive>& primitives)
{
    m_primitives = primitives;
    // BVH::Build reorders m_primitives in place; brute-force traversal over the
    // reordered list still yields the same closest-hit / occlusion result.
    m_bvh.Build(m_primitives);
}

bool RayScene::Intersect(const Ray& ray, HitRecord& hit, float tMin, float tMax) const
{
    if (UsesBVH())
    {
        return m_bvh.Intersect(ray, hit, tMin, tMax);
    }

    bool hitAnything = false;
    float closest = tMax;

    for (const TracePrimitive& primitive : m_primitives)
    {
        HitRecord tempHit;
        if (primitive.Intersect(ray, tMin, closest, tempHit))
        {
            hitAnything = true;
            closest = tempHit.t;
            hit = tempHit;
        }
    }

    return hitAnything;
}

bool RayScene::Occluded(const Ray& ray, float tMin, float tMax) const
{
    if (UsesBVH())
    {
        return m_bvh.Occluded(ray, tMin, tMax);
    }

    for (const TracePrimitive& primitive : m_primitives)
    {
        HitRecord tempHit;
        if (primitive.Intersect(ray, tMin, tMax, tempHit))
        {
            return true;
        }
    }

    return false;
}
} // namespace hr
