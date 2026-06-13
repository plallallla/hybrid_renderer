#include "rasterizer.h"
#include "../../renderer/material_evaluator.h"
#include "../../renderer/pbr.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace hr
{
namespace
{
struct ProjectedVertex
{
    Vec3 worldPosition;
    Vec3 normal;
    Vec2 uv;
    Vec4 clipPosition;
    float invW = 1.0f;
    float screenX = 0.0f;
    float screenY = 0.0f;
    float screenZ = 0.0f;
};

// Per-pixel fragment produced by the shared rasterization core. Holds the
// perspective-correct interpolated surface attributes plus the resolved
// material, so that both Render() (lighting) and RenderGBuffer() (attribute
// capture) can consume the same source of truth without duplicating the
// rasterization loop (see render.md §21).
struct Fragment
{
    Vec3 worldPosition;
    Vec3 normal;
    Vec2 uv;
    Vec3 tangent;
    Vec3 bitangent;
    float depth = 0.0f;
    int materialId = -1;
    const Material* material = nullptr;
};

ProjectedVertex ProjectVertex(const Vertex& vertex, const Mat4& mvp, const Mat4& model, int width, int height)
{
    ProjectedVertex result;
    result.worldPosition = TransformPoint(model, vertex.position);
    result.normal = Normalize(TransformDirection(model, vertex.normal));
    result.uv = vertex.uv;
    result.clipPosition = mvp * Vec4{vertex.position.x, vertex.position.y, vertex.position.z, 1.0f};
    result.invW = 1.0f / result.clipPosition.w;
    const float ndcX = result.clipPosition.x * result.invW;
    const float ndcY = result.clipPosition.y * result.invW;
    const float ndcZ = result.clipPosition.z * result.invW;
    result.screenX = (ndcX * 0.5f + 0.5f) * static_cast<float>(width - 1);
    result.screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * static_cast<float>(height - 1);
    result.screenZ = ndcZ * 0.5f + 0.5f;
    return result;
}

void ComputeTangentFrame(
    const ProjectedVertex& v0,
    const ProjectedVertex& v1,
    const ProjectedVertex& v2,
    Vec3& tangent,
    Vec3& bitangent)
{
    const Vec3 edge1 = v1.worldPosition - v0.worldPosition;
    const Vec3 edge2 = v2.worldPosition - v0.worldPosition;
    const Vec2 deltaUV1 = v1.uv - v0.uv;
    const Vec2 deltaUV2 = v2.uv - v0.uv;

    const float denom = deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x;
    if (std::abs(denom) < 1e-6f)
    {
        tangent = {1.0f, 0.0f, 0.0f};
        bitangent = {0.0f, 1.0f, 0.0f};
        return;
    }

    const float inv = 1.0f / denom;
    tangent = Normalize((edge1 * deltaUV2.y - edge2 * deltaUV1.y) * inv);
    bitangent = Normalize((edge2 * deltaUV1.x - edge1 * deltaUV2.x) * inv);
}

float EdgeFunction(float ax, float ay, float bx, float by, float cx, float cy)
{
    return (cx - ax) * (by - ay) - (cy - ay) * (bx - ax);
}

// Shared rasterization core: walks every mesh/triangle, performs perspective-
// correct interpolation and depth testing against `depthBuffer`, and invokes
// `onFragment(x, y, Fragment)` for each pixel that passes the depth test.
// Both Render() and RenderGBuffer() build on this so the visibility logic
// exists in exactly one place.
template <typename FragmentFn>
void RasterizeScene(
    const Scene& scene,
    const Camera& camera,
    const RenderSettings& settings,
    Image<float>& depthBuffer,
    FragmentFn&& onFragment)
{
    const float aspect = static_cast<float>(settings.width) / static_cast<float>(settings.height);
    const Mat4 view = camera.ViewMatrix();
    const Mat4 projection = camera.ProjectionMatrix(aspect);

    for (const Mesh& mesh : scene.meshes)
    {
        const Material defaultMaterial{};
        const Material* material = &defaultMaterial;
        int materialId = -1;
        if (!scene.materials.empty() && mesh.materialId >= 0 && mesh.materialId < static_cast<int>(scene.materials.size()))
        {
            material = &scene.materials[static_cast<size_t>(mesh.materialId)];
            materialId = mesh.materialId;
        }
        const Mat4 modelViewProjection = projection * view * mesh.transform;

        for (const Triangle& triangle : mesh.triangles)
        {
            const ProjectedVertex v0 = ProjectVertex(mesh.vertices[static_cast<size_t>(triangle.a)], modelViewProjection, mesh.transform, settings.width, settings.height);
            const ProjectedVertex v1 = ProjectVertex(mesh.vertices[static_cast<size_t>(triangle.b)], modelViewProjection, mesh.transform, settings.width, settings.height);
            const ProjectedVertex v2 = ProjectVertex(mesh.vertices[static_cast<size_t>(triangle.c)], modelViewProjection, mesh.transform, settings.width, settings.height);
            Vec3 tangent;
            Vec3 bitangent;
            ComputeTangentFrame(v0, v1, v2, tangent, bitangent);

            const float area = EdgeFunction(v0.screenX, v0.screenY, v1.screenX, v1.screenY, v2.screenX, v2.screenY);
            if (std::abs(area) < 1e-6f)
            {
                continue;
            }

            const int minX = std::max(0, static_cast<int>(std::floor(std::min({v0.screenX, v1.screenX, v2.screenX}))));
            const int minY = std::max(0, static_cast<int>(std::floor(std::min({v0.screenY, v1.screenY, v2.screenY}))));
            const int maxX = std::min(settings.width - 1, static_cast<int>(std::ceil(std::max({v0.screenX, v1.screenX, v2.screenX}))));
            const int maxY = std::min(settings.height - 1, static_cast<int>(std::ceil(std::max({v0.screenY, v1.screenY, v2.screenY}))));

            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    const float px = static_cast<float>(x) + 0.5f;
                    const float py = static_cast<float>(y) + 0.5f;

                    const float w0 = EdgeFunction(v1.screenX, v1.screenY, v2.screenX, v2.screenY, px, py) / area;
                    const float w1 = EdgeFunction(v2.screenX, v2.screenY, v0.screenX, v0.screenY, px, py) / area;
                    const float w2 = 1.0f - w0 - w1;
                    if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f)
                    {
                        continue;
                    }

                    const float invW = w0 * v0.invW + w1 * v1.invW + w2 * v2.invW;
                    const float depth = (w0 * v0.screenZ * v0.invW + w1 * v1.screenZ * v1.invW + w2 * v2.screenZ * v2.invW) / invW;

                    float& currentDepth = depthBuffer.At(x, y);
                    if (depth >= currentDepth)
                    {
                        continue;
                    }

                    currentDepth = depth;

                    Fragment fragment;
                    fragment.depth = depth;
                    fragment.material = material;
                    fragment.materialId = materialId;
                    fragment.tangent = tangent;
                    fragment.bitangent = bitangent;
                    fragment.uv = {
                        (w0 * v0.uv.x * v0.invW + w1 * v1.uv.x * v1.invW + w2 * v2.uv.x * v2.invW) / invW,
                        (w0 * v0.uv.y * v0.invW + w1 * v1.uv.y * v1.invW + w2 * v2.uv.y * v2.invW) / invW,
                    };
                    fragment.normal = Normalize({
                        (w0 * v0.normal.x * v0.invW + w1 * v1.normal.x * v1.invW + w2 * v2.normal.x * v2.invW) / invW,
                        (w0 * v0.normal.y * v0.invW + w1 * v1.normal.y * v1.invW + w2 * v2.normal.y * v2.invW) / invW,
                        (w0 * v0.normal.z * v0.invW + w1 * v1.normal.z * v1.invW + w2 * v2.normal.z * v2.invW) / invW,
                    });
                    fragment.worldPosition = {
                        (w0 * v0.worldPosition.x * v0.invW + w1 * v1.worldPosition.x * v1.invW + w2 * v2.worldPosition.x * v2.invW) / invW,
                        (w0 * v0.worldPosition.y * v0.invW + w1 * v1.worldPosition.y * v1.invW + w2 * v2.worldPosition.y * v2.invW) / invW,
                        (w0 * v0.worldPosition.z * v0.invW + w1 * v1.worldPosition.z * v1.invW + w2 * v2.worldPosition.z * v2.invW) / invW,
                    };

                    onFragment(x, y, fragment);
                }
            }
        }
    }
}

Vec3 ShadePixel(
    const Scene& scene,
    const Camera& camera,
    const Material& material,
    const Vec3& worldPosition,
    const Vec2& uv,
    const Vec3& normal,
    const Vec3& tangent,
    const Vec3& bitangent)
{
    const MaterialSample sample = MaterialEvaluator::Evaluate(scene, material, uv, normal, tangent, bitangent);
    const Vec3 viewDir = Normalize(camera.position - worldPosition);
    Vec3 color{0.0f, 0.0f, 0.0f};

    for (const DirectionalLight& light : scene.lights)
    {
        const Vec3 lightDir = Normalize(-light.direction);
        const Vec3 lightRadiance = light.color * light.intensity;
        color = color + EvaluatePBRDirect(sample, sample.normal, viewDir, lightDir, lightRadiance);
    }

    color = color + sample.baseColor * 0.03f;
    return Clamp(color, 0.0f, 1.0f);
}
} // namespace

void SoftwareRasterizer::Render(const Scene& scene, const Camera& camera, Framebuffer& output, const RenderSettings& settings)
{
    output.Resize(settings.width, settings.height);
    output.Clear(scene.background);

    RasterizeScene(scene, camera, settings, output.depth, [&](int x, int y, const Fragment& fragment) {
        output.color.At(x, y) = ShadePixel(
            scene,
            camera,
            *fragment.material,
            fragment.worldPosition,
            fragment.uv,
            fragment.normal,
            fragment.tangent,
            fragment.bitangent);
    });
}

void SoftwareRasterizer::RenderGBuffer(const Scene& scene, const Camera& camera, GBuffer& gbuffer, const RenderSettings& settings)
{
    gbuffer.Resize(settings.width, settings.height);

    RasterizeScene(scene, camera, settings, gbuffer.depth, [&](int x, int y, const Fragment& fragment) {
        const MaterialSample sample = MaterialEvaluator::Evaluate(
            scene,
            *fragment.material,
            fragment.uv,
            fragment.normal,
            fragment.tangent,
            fragment.bitangent);

        gbuffer.position.At(x, y) = fragment.worldPosition;
        gbuffer.normal.At(x, y) = sample.normal;
        gbuffer.baseColor.At(x, y) = sample.baseColor;
        gbuffer.metallic.At(x, y) = sample.metallic;
        gbuffer.roughness.At(x, y) = sample.roughness;
        gbuffer.materialID.At(x, y) = fragment.materialId;
        // Image<bool> stores std::vector<bool>, whose element access yields a
        // proxy rather than a bool&, so write coverage through Data() instead
        // of At() to keep the proxy assignment valid.
        gbuffer.valid.Data()[static_cast<std::size_t>(y) * static_cast<std::size_t>(settings.width) + static_cast<std::size_t>(x)] = true;
    });
}
} // namespace hr
