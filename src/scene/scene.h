#pragma once

#include "../geometry/sphere.h"
#include "camera.h"
#include "light.h"
#include "material.h"
#include "mesh.h"
#include "texture.h"

#include <vector>

namespace hr
{
struct Scene
{
    Camera camera;
    Vec3 background{0.0f, 0.0f, 0.0f};
    std::vector<Material> materials;
    std::vector<Texture2D> textures;
    std::vector<Mesh> meshes;
    std::vector<Sphere> spheres;
    std::vector<DirectionalLight> lights;
    std::vector<PointLight> pointLights;
};
} // namespace hr
