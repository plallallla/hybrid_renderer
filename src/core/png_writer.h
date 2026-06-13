#pragma once

#include "image.h"
#include "math.h"

#include <cstdint>
#include <string>

namespace hr
{
bool WritePng(const std::string& path, const Image<Vec3>& image);
} // namespace hr
