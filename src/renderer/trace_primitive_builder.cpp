#include "trace_primitive_builder.h"

namespace hr
{
std::vector<TracePrimitive> BuildTracePrimitives(const Scene& scene)
{
    std::vector<TracePrimitive> primitives;

    for (const Mesh& mesh : scene.meshes)
    {
        for (const Triangle& triangle : mesh.triangles)
        {
            const Vertex& a = mesh.vertices[static_cast<size_t>(triangle.a)];
            const Vertex& b = mesh.vertices[static_cast<size_t>(triangle.b)];
            const Vertex& c = mesh.vertices[static_cast<size_t>(triangle.c)];

            TracePrimitive primitive;
            primitive.type = TracePrimitiveType::Triangle;
            primitive.triangle.v0 = {TransformPoint(mesh.transform, a.position), Normalize(TransformDirection(mesh.transform, a.normal)), a.uv};
            primitive.triangle.v1 = {TransformPoint(mesh.transform, b.position), Normalize(TransformDirection(mesh.transform, b.normal)), b.uv};
            primitive.triangle.v2 = {TransformPoint(mesh.transform, c.position), Normalize(TransformDirection(mesh.transform, c.normal)), c.uv};
            primitive.materialID = mesh.materialId;
            primitive.primitiveID = static_cast<int>(primitives.size());
            primitives.push_back(primitive);
        }
    }

    for (const Sphere& sphere : scene.spheres)
    {
        TracePrimitive primitive;
        primitive.type = TracePrimitiveType::Sphere;
        primitive.sphere = sphere;
        primitive.materialID = sphere.materialId;
        primitive.primitiveID = static_cast<int>(primitives.size());
        primitives.push_back(primitive);
    }

    return primitives;
}
} // namespace hr
