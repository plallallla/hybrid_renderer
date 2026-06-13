#pragma once

#include "image.h"
#include "math.h"

#include <limits>

namespace hr
{
class Framebuffer
{
public:
    void Resize(int width, int height)
    {
        color.Resize(width, height, {0.0f, 0.0f, 0.0f});
        depth.Resize(width, height, std::numeric_limits<float>::infinity());
    }

    void Clear(const Vec3& clearColor)
    {
        for (auto& pixel : color.Data())
        {
            pixel = clearColor;
        }

        for (auto& value : depth.Data())
        {
            value = std::numeric_limits<float>::infinity();
        }
    }

    Image<Vec3> color;
    Image<float> depth;
};
} // namespace hr
