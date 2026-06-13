#pragma once

#include "../core/math.h"

namespace hr
{
class Camera
{
public:
    Vec3 position{0.0f, 0.0f, 3.0f};
    Vec3 target{0.0f, 0.0f, 0.0f};
    Vec3 up{0.0f, 1.0f, 0.0f};
    float fovYDegrees = 60.0f;
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    Mat4 ViewMatrix() const
    {
        return Mat4::LookAt(position, target, up);
    }

    Mat4 ProjectionMatrix(float aspectRatio) const
    {
        return Mat4::Perspective(Radians(fovYDegrees), aspectRatio, nearPlane, farPlane);
    }

    Ray GenerateRay(float u, float v) const
    {
        const Vec3 forward = Normalize(target - position);
        const Vec3 right = Normalize(Cross(forward, up));
        const Vec3 cameraUp = Normalize(Cross(right, forward));

        const float tanHalfFov = std::tan(Radians(fovYDegrees) * 0.5f);

        const float px = (2.0f * u - 1.0f) * aspect * tanHalfFov;
        const float py = (1.0f - 2.0f * v) * tanHalfFov;

        const Vec3 direction = Normalize(forward + right * px + cameraUp * py);

        return Ray{position, direction};
    }
};
} // namespace hr
