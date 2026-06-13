#pragma once

#include "../scene/scene.h"

#include <string>

namespace hr
{
bool LoadSceneFromJson(const std::string& path, Scene& scene);
Scene CreateDefaultScene();
} // namespace hr
