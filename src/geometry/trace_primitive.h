#pragma once

#include "aabb.h"
#include "hit_record.h"
#include "ray.h"
#include "sphere.h"

namespace hr
{
struct TraceVertex
{
    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{0.0f, 0.0f};
};

struct TraceTriangle
{
    TraceVertex v0;
    TraceVertex v1;
    TraceVertex v2;
};

enum class TracePrimitiveType
{
    Triangle,
    Sphere
};

struct TracePrimitive
{
    TracePrimitiveType type = TracePrimitiveType::Triangle;

    TraceTriangle triangle;
    Sphere sphere;

    int materialID = -1;
    int primitiveID = -1;

    AABB Bounds() const;
    Vec3 Center() const;

    bool Intersect(const Ray& ray, float tMin, float tMax, HitRecord& hit) const;
};
} // namespace hr
