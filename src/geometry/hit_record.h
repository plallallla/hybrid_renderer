#pragma once

#include "ray.h"

#include <limits>

namespace hr
{
struct HitRecord
{
    float t = std::numeric_limits<float>::max();

    Vec3 position{0.0f, 0.0f, 0.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    Vec2 uv{0.0f, 0.0f};

    int materialID = -1;
    int primitiveID = -1;

    bool frontFace = true;

    void SetFaceNormal(const Ray& ray, const Vec3& outwardNormal)
    {
        frontFace = Dot(ray.direction, outwardNormal) < 0.0f;
        normal = frontFace ? outwardNormal : -outwardNormal;
    }
};
} // namespace hr
