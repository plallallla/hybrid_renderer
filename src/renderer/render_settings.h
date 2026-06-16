#pragma once

namespace hr
{
struct RenderSettings
{
    int width = 800;
    int height = 600;
    int samplesPerPixel = 5;
    int maxDepth = 16;

    bool enableRayTracedShadow = true;
    bool enableRayTracedAO = false;
    bool enableRayTracedReflection = false;

    int aoSamples = 8;
    float aoRadius = 1.0f;

    // Shadow-ray samples per rectangular area light (Hybrid soft shadows).
    int areaLightSamples = 16;

    float exposure = 1.0f;
    bool enableGammaCorrection = true;

    // Acceleration structure toggle for PathTracer / Hybrid ray queries.
    // Disable to fall back to brute-force traversal (BVH-vs-brute comparison).
    bool useBVH = true;
};
} // namespace hr
