#pragma once

#include "mesh.h"

#include <string>

namespace hr
{
bool LoadObj(const std::string& path, Mesh& mesh);
} // namespace hr
