#pragma once

#include "../core/image.h"
#include "../core/math.h"

#include <limits>

namespace hr
{
struct GBuffer
{
    Image<Vec3> position;
    Image<Vec3> normal;
    Image<Vec3> baseColor;
    Image<float> depth;
    Image<float> metallic;
    Image<float> roughness;
    Image<int> materialID;
    Image<bool> valid;

    void Resize(int width, int height)
    {
        position.Resize(width, height, {0.0f, 0.0f, 0.0f});
        normal.Resize(width, height, {0.0f, 1.0f, 0.0f});
        baseColor.Resize(width, height, {1.0f, 1.0f, 1.0f});
        depth.Resize(width, height, std::numeric_limits<float>::infinity());
        metallic.Resize(width, height, 0.0f);
        roughness.Resize(width, height, 0.5f);
        materialID.Resize(width, height, -1);
        valid.Resize(width, height, false);
    }

    void Clear()
    {
        const int width = depth.Width();
        const int height = depth.Height();
        Resize(width, height);
    }
};
} // namespace hr
