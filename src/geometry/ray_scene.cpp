#include "ray_scene.h"

namespace hr
{
void RayScene::Build(const std::vector<TracePrimitive>& primitives)
{
    m_primitives = primitives;
}

bool RayScene::Intersect(const Ray& ray, HitRecord& hit, float tMin, float tMax) const
{
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
