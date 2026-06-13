#pragma once

#include "../core/math.h"
#include "material_evaluator.h"

#include <algorithm>
#include <cmath>

namespace hr
{
inline Vec3 FresnelSchlick(float cosTheta, const Vec3& F0)
{
    const float t = std::pow(Clamp(1.0f - cosTheta, 0.0f, 1.0f), 5.0f);
    return F0 + (Vec3{1.0f, 1.0f, 1.0f} - F0) * t;
}

inline float DistributionGGX(float NdotH, float roughness)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdotH2 = NdotH * NdotH;
    const float denom = NdotH2 * (a2 - 1.0f) + 1.0f;
    return a2 / (PI * denom * denom + 1e-7f);
}

inline float GeometrySchlickGGX(float NdotX, float roughness)
{
    const float r = roughness + 1.0f;
    const float k = (r * r) / 8.0f;
    return NdotX / (NdotX * (1.0f - k) + k);
}

inline float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

inline Vec3 EvaluatePBRDirect(
    const MaterialSample& material,
    const Vec3& N,
    const Vec3& V,
    const Vec3& L,
    const Vec3& lightRadiance)
{
    const float NdotL = std::max(0.0f, Dot(N, L));
    const float NdotV = std::max(0.0f, Dot(N, V));
    if (NdotL <= 0.0f || NdotV <= 0.0f)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    const Vec3 H = Normalize(V + L);
    const float NdotH = std::max(0.0f, Dot(N, H));
    const float HdotV = std::max(0.0f, Dot(H, V));

    const Vec3 F0 = Lerp(Vec3{0.04f, 0.04f, 0.04f}, material.baseColor, material.metallic);
    const Vec3 F = FresnelSchlick(HdotV, F0);
    const float D = DistributionGGX(NdotH, material.roughness);
    const float G = GeometrySmith(NdotV, NdotL, material.roughness);

    const Vec3 specular = F * (D * G / (4.0f * NdotV * NdotL));

    // Energy-conserving diffuse: kD = (1 - F) * (1 - metallic), matching the
    // reference shader (kill_cow) so specular Fresnel removes that energy from
    // the diffuse lobe.
    const Vec3 kD = (Vec3{1.0f, 1.0f, 1.0f} - F) * (1.0f - material.metallic);
    const Vec3 diffuse = kD * material.baseColor * INV_PI;

    return (diffuse + specular) * lightRadiance * NdotL;
}
} // namespace hr
