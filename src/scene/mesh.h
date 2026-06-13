#pragma once

#include "../core/math.h"

#include <vector>

namespace hr
{
struct Vertex
{
    Vec3 position;
    Vec3 normal;
    Vec2 uv;
};

struct Triangle
{
    int a = 0;
    int b = 0;
    int c = 0;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<Triangle> triangles;
    int materialId = 0;
    Mat4 transform = Mat4::Identity();
};
} // namespace hr
