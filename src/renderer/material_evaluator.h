#pragma once

#include "../scene/material.h"
#include "../scene/scene.h"

#include <algorithm>
#include <cmath>

namespace hr
{
struct MaterialSample
{
    Vec3 baseColor{1.0f, 1.0f, 1.0f};
    Vec3 normal{0.0f, 1.0f, 0.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    Vec3 emission{0.0f, 0.0f, 0.0f};
};

class MaterialEvaluator
{
public:
    static MaterialSample Evaluate(
        const Scene& scene,
        const Material& material,
        const Vec2& uv,
        const Vec3& geometricNormal,
        const Vec3& tangent,
        const Vec3& bitangent)
    {
        MaterialSample sample;
        sample.baseColor = material.baseColor;
        sample.normal = Normalize(geometricNormal);
        sample.metallic = material.metallic;
        sample.roughness = Clamp(material.roughness, 0.04f, 1.0f);
        sample.emission = material.emission;

        if (material.baseColorTexture >= 0 && material.baseColorTexture < static_cast<int>(scene.textures.size()))
        {
            // Albedo maps are authored in sRGB; decode to linear before lighting
            // (reference shader does pow(albedo, 2.2)). Other maps (normal /
            // metallic / roughness / ao) are linear data and stay untouched.
            const Vec3 srgb = scene.textures[static_cast<size_t>(material.baseColorTexture)].Sample(uv.x, uv.y);
            sample.baseColor = {
                std::pow(srgb.x, 2.2f),
                std::pow(srgb.y, 2.2f),
                std::pow(srgb.z, 2.2f),
            };
        }

        if (material.metallicRoughnessTexture >= 0 && material.metallicRoughnessTexture < static_cast<int>(scene.textures.size()))
        {
            const Vec3 packed = scene.textures[static_cast<size_t>(material.metallicRoughnessTexture)].Sample(uv.x, uv.y);
            sample.roughness = Clamp(packed.y, 0.04f, 1.0f);
            sample.metallic = packed.z;
        }

        if (material.occlusionTexture >= 0 && material.occlusionTexture < static_cast<int>(scene.textures.size()))
        {
            sample.ao = scene.textures[static_cast<size_t>(material.occlusionTexture)].Sample(uv.x, uv.y).x;
        }

        if (material.normalTexture >= 0 && material.normalTexture < static_cast<int>(scene.textures.size()))
        {
            const Vec3 encoded = scene.textures[static_cast<size_t>(material.normalTexture)].Sample(uv.x, uv.y);
            const Vec3 tangentNormal = Normalize({
                encoded.x * 2.0f - 1.0f,
                encoded.y * 2.0f - 1.0f,
                encoded.z * 2.0f - 1.0f,
            });
            const Vec3 T = Normalize(tangent);
            const Vec3 B = Normalize(bitangent);
            const Vec3 N = Normalize(geometricNormal);
            sample.normal = Normalize(
                T * tangentNormal.x +
                B * tangentNormal.y +
                N * tangentNormal.z);
        }

        return sample;
    }
};
} // namespace hr
