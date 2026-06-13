#include "trace_primitive.h"

#include <cmath>

namespace hr
{
namespace
{
bool IntersectTriangle(const Ray& ray, const TraceTriangle& tri, float tMin, float tMax, HitRecord& hit)
{
    const Vec3& p0 = tri.v0.position;
    const Vec3& p1 = tri.v1.position;
    const Vec3& p2 = tri.v2.position;

    const Vec3 edge1 = p1 - p0;
    const Vec3 edge2 = p2 - p0;

    const Vec3 pvec = Cross(ray.direction, edge2);
    const float det = Dot(edge1, pvec);

    if (std::abs(det) < EPSILON)
    {
        return false;
    }

    const float invDet = 1.0f / det;
    const Vec3 tvec = ray.origin - p0;

    const float u = Dot(tvec, pvec) * invDet;
    if (u < 0.0f || u > 1.0f)
    {
        return false;
    }

    const Vec3 qvec = Cross(tvec, edge1);
    const float v = Dot(ray.direction, qvec) * invDet;
    if (v < 0.0f || u + v > 1.0f)
    {
        return false;
    }

    const float t = Dot(edge2, qvec) * invDet;
    if (t < tMin || t > tMax)
    {
        return false;
    }

    const float w = 1.0f - u - v;

    hit.t = t;
    hit.position = ray.At(t);

    const Vec3 interpolatedNormal = Normalize(tri.v0.normal * w + tri.v1.normal * u + tri.v2.normal * v);
    hit.SetFaceNormal(ray, interpolatedNormal);

    hit.uv = tri.v0.uv * w + tri.v1.uv * u + tri.v2.uv * v;

    return true;
}

bool IntersectSphere(const Ray& ray, const Sphere& sphere, float tMin, float tMax, HitRecord& hit)
{
    const Vec3 oc = ray.origin - sphere.center;

    const float a = Dot(ray.direction, ray.direction);
    const float halfB = Dot(oc, ray.direction);
    const float c = Dot(oc, oc) - sphere.radius * sphere.radius;

    const float discriminant = halfB * halfB - a * c;
    if (discriminant < 0.0f)
    {
        return false;
    }

    const float sqrtD = std::sqrt(discriminant);

    float root = (-halfB - sqrtD) / a;
    if (root < tMin || root > tMax)
    {
        root = (-halfB + sqrtD) / a;
        if (root < tMin || root > tMax)
        {
            return false;
        }
    }

    hit.t = root;
    hit.position = ray.At(root);

    const Vec3 outwardNormal = (hit.position - sphere.center) / sphere.radius;
    hit.SetFaceNormal(ray, outwardNormal);

    return true;
}
} // namespace

AABB TracePrimitive::Bounds() const
{
    if (type == TracePrimitiveType::Sphere)
    {
        return sphere.Bounds();
    }

    AABB bounds;
    bounds.Expand(triangle.v0.position);
    bounds.Expand(triangle.v1.position);
    bounds.Expand(triangle.v2.position);
    return bounds;
}

Vec3 TracePrimitive::Center() const
{
    if (type == TracePrimitiveType::Sphere)
    {
        return sphere.center;
    }

    return (triangle.v0.position + triangle.v1.position + triangle.v2.position) / 3.0f;
}

bool TracePrimitive::Intersect(const Ray& ray, float tMin, float tMax, HitRecord& hit) const
{
    bool result = false;

    if (type == TracePrimitiveType::Triangle)
    {
        result = IntersectTriangle(ray, triangle, tMin, tMax, hit);
    }
    else
    {
        result = IntersectSphere(ray, sphere, tMin, tMax, hit);
    }

    if (result)
    {
        hit.materialID = materialID;
        hit.primitiveID = primitiveID;
    }

    return result;
}
} // namespace hr
