#pragma once

#include "math.h"

#include <cstdint>
#include <random>

namespace hr
{
class Random
{
public:
    explicit Random(uint32_t seed = 0)
        : m_engine(seed)
        , m_distribution(0.0f, 1.0f)
    {
    }

    float NextFloat()
    {
        return m_distribution(m_engine);
    }

    Vec2 NextVec2()
    {
        return {NextFloat(), NextFloat()};
    }

private:
    std::mt19937 m_engine;
    std::uniform_real_distribution<float> m_distribution;
};
} // namespace hr
