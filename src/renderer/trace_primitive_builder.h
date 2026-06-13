#pragma once

#include "../geometry/trace_primitive.h"
#include "../scene/scene.h"

#include <vector>

namespace hr
{
std::vector<TracePrimitive> BuildTracePrimitives(const Scene& scene);
} // namespace hr
