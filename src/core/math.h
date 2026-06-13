#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace hr
{
constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 6.28318530717958647692f;
constexpr float INV_PI = 0.31830988618379067154f;
constexpr float EPSILON = 1e-4f;

struct Vec2
{
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    float operator[](int axis) const
    {
        switch (axis)
        {
            case 0: return x;
            case 1: return y;
            default: return z;
        }
    }

    float& operator[](int axis)
    {
        switch (axis)
        {
            case 0: return x;
            case 1: return y;
            default: return z;
        }
    }
};

struct Vec4
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

inline Vec2 operator+(const Vec2& a, const Vec2& b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator-(const Vec2& a, const Vec2& b) { return {a.x - b.x, a.y - b.y}; }
inline Vec2 operator*(const Vec2& a, float s) { return {a.x * s, a.y * s}; }

inline Vec3 operator+(const Vec3& a, const Vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(const Vec3& a, const Vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(const Vec3& a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(const Vec3& a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, const Vec3& a) { return a * s; }
inline Vec3 operator/(const Vec3& a, float s) { return {a.x / s, a.y / s, a.z / s}; }
inline Vec3 operator*(const Vec3& a, const Vec3& b) { return {a.x * b.x, a.y * b.y, a.z * b.z}; }

inline Vec4 operator+(const Vec4& a, const Vec4& b) { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
inline Vec4 operator*(const Vec4& a, float s) { return {a.x * s, a.y * s, a.z * s, a.w * s}; }

inline float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }
inline float Dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 Cross(const Vec3& a, const Vec3& b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

inline float Length(const Vec3& v)
{
    return std::sqrt(Dot(v, v));
}

inline Vec3 Normalize(const Vec3& v)
{
    const float len = Length(v);
    if (len <= 0.0f)
    {
        return {0.0f, 0.0f, 0.0f};
    }
    return v / len;
}

inline float Clamp(float v, float lo, float hi)
{
    return std::max(lo, std::min(v, hi));
}

inline int Clamp(int v, int lo, int hi)
{
    return std::max(lo, std::min(v, hi));
}

inline Vec3 Clamp(const Vec3& v, float lo, float hi)
{
    return {Clamp(v.x, lo, hi), Clamp(v.y, lo, hi), Clamp(v.z, lo, hi)};
}

inline Vec3 Max(const Vec3& a, const Vec3& b)
{
    return {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)};
}

inline Vec3 Min(const Vec3& a, const Vec3& b)
{
    return {std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)};
}

inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t)
{
    return a * (1.0f - t) + b * t;
}

inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline Vec3 Reflect(const Vec3& incident, const Vec3& normal)
{
    return incident - normal * (2.0f * Dot(incident, normal));
}

inline float Radians(float degrees)
{
    return degrees * PI / 180.0f;
}

struct Mat4
{
    std::array<float, 16> m{};

    static Mat4 Identity()
    {
        Mat4 result;
        result.m = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        return result;
    }

    static Mat4 Translation(const Vec3& t)
    {
        Mat4 result = Identity();
        result.m[3] = t.x;
        result.m[7] = t.y;
        result.m[11] = t.z;
        return result;
    }

    static Mat4 Scale(const Vec3& s)
    {
        Mat4 result = Identity();
        result.m[0] = s.x;
        result.m[5] = s.y;
        result.m[10] = s.z;
        return result;
    }

    static Mat4 Perspective(float fovYRadians, float aspect, float nearPlane, float farPlane)
    {
        Mat4 result{};
        const float f = 1.0f / std::tan(fovYRadians * 0.5f);
        result.m = {
            f / aspect, 0.0f, 0.0f, 0.0f,
            0.0f, f, 0.0f, 0.0f,
            0.0f, 0.0f, (farPlane + nearPlane) / (nearPlane - farPlane), (2.0f * farPlane * nearPlane) / (nearPlane - farPlane),
            0.0f, 0.0f, -1.0f, 0.0f,
        };
        return result;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& target, const Vec3& up)
    {
        const Vec3 f = Normalize(target - eye);
        const Vec3 s = Normalize(Cross(f, up));
        const Vec3 u = Cross(s, f);

        Mat4 result = Identity();
        result.m = {
            s.x, s.y, s.z, -Dot(s, eye),
            u.x, u.y, u.z, -Dot(u, eye),
            -f.x, -f.y, -f.z, Dot(f, eye),
            0.0f, 0.0f, 0.0f, 1.0f,
        };
        return result;
    }
};

inline Mat4 operator*(const Mat4& a, const Mat4& b)
{
    Mat4 result{};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                sum += a.m[row * 4 + k] * b.m[k * 4 + col];
            }
            result.m[row * 4 + col] = sum;
        }
    }
    return result;
}

inline Vec4 operator*(const Mat4& m, const Vec4& v)
{
    return {
        m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z + m.m[3] * v.w,
        m.m[4] * v.x + m.m[5] * v.y + m.m[6] * v.z + m.m[7] * v.w,
        m.m[8] * v.x + m.m[9] * v.y + m.m[10] * v.z + m.m[11] * v.w,
        m.m[12] * v.x + m.m[13] * v.y + m.m[14] * v.z + m.m[15] * v.w,
    };
}

inline Vec3 TransformPoint(const Mat4& m, const Vec3& v)
{
    const Vec4 result = m * Vec4{v.x, v.y, v.z, 1.0f};
    if (std::abs(result.w) > 1e-6f)
    {
        return {result.x / result.w, result.y / result.w, result.z / result.w};
    }
    return {result.x, result.y, result.z};
}

inline Vec3 TransformDirection(const Mat4& m, const Vec3& v)
{
    return {
        m.m[0] * v.x + m.m[1] * v.y + m.m[2] * v.z,
        m.m[4] * v.x + m.m[5] * v.y + m.m[6] * v.z,
        m.m[8] * v.x + m.m[9] * v.y + m.m[10] * v.z,
    };
}

struct Ray
{
    Vec3 origin;
    Vec3 direction;

    Vec3 At(float t) const
    {
        return origin + direction * t;
    }
};
} // namespace hr
