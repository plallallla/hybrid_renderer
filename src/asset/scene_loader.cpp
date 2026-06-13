#include "scene_loader.h"

#include "json.h"
#include "../scene/obj_loader.h"

#include <cmath>
#include <filesystem>
#include <utility>

namespace hr
{
namespace
{
std::string ResolveRelativePath(const std::filesystem::path& baseDir, const std::string& path)
{
    std::filesystem::path resolved(path);
    if (resolved.is_absolute())
    {
        return resolved.string();
    }
    return (baseDir / resolved).string();
}

Vec3 ReadVec3(const JsonValue* value, const Vec3& fallback)
{
    if (!value || !value->IsArray() || value->AsArray().size() < 3)
    {
        return fallback;
    }
    return {
        static_cast<float>(value->AsArray()[0].AsNumber(fallback.x)),
        static_cast<float>(value->AsArray()[1].AsNumber(fallback.y)),
        static_cast<float>(value->AsArray()[2].AsNumber(fallback.z)),
    };
}

Vec2 ReadVec2(const JsonValue* value, const Vec2& fallback)
{
    if (!value || !value->IsArray() || value->AsArray().size() < 2)
    {
        return fallback;
    }
    return {
        static_cast<float>(value->AsArray()[0].AsNumber(fallback.x)),
        static_cast<float>(value->AsArray()[1].AsNumber(fallback.y)),
    };
}

Mat4 ComposeTransform(const JsonValue* value)
{
    if (!value || !value->IsObject())
    {
        return Mat4::Identity();
    }

    const Vec3 translation = ReadVec3(value->Find("translation"), {0.0f, 0.0f, 0.0f});
    const Vec3 scale = ReadVec3(value->Find("scale"), {1.0f, 1.0f, 1.0f});
    return Mat4::Translation(translation) * Mat4::Scale(scale);
}
} // namespace

Scene CreateDefaultScene()
{
    Scene scene;
    scene.background = {0.02f, 0.02f, 0.03f};
    scene.camera.position = {0.0f, 0.0f, 3.0f};
    scene.camera.target = {0.0f, 0.0f, 0.0f};
    scene.camera.up = {0.0f, 1.0f, 0.0f};

    scene.materials.push_back(Material{});
    scene.lights.push_back(DirectionalLight{});

    Texture2D checker;
    if (checker.LoadFromFile("assets/textures/checker.ppm"))
    {
        scene.materials[0].baseColorTexture = static_cast<int>(scene.textures.size());
        scene.textures.push_back(std::move(checker));
    }
    scene.materials[0].metallic = 0.0f;
    scene.materials[0].roughness = 0.4f;

    Mesh mesh;
    mesh.materialId = 0;
    mesh.vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
    };
    mesh.triangles.push_back({0, 1, 2});
    scene.meshes.push_back(mesh);
    return scene;
}

bool LoadSceneFromJson(const std::string& path, Scene& scene)
{
    JsonValue root;
    if (!LoadJsonFile(path, root) || !root.IsObject())
    {
        return false;
    }

    const std::filesystem::path scenePath(path);
    const std::filesystem::path baseDir = scenePath.parent_path();
    scene = Scene{};
    if (const JsonValue* background = root.Find("background"))
    {
        scene.background = ReadVec3(background, scene.background);
    }
    else
    {
        scene.background = {0.02f, 0.02f, 0.03f};
    }

    if (const JsonValue* camera = root.Find("camera"))
    {
        if (camera->IsObject())
        {
            scene.camera.position = ReadVec3(camera->Find("position"), scene.camera.position);
            scene.camera.target = ReadVec3(camera->Find("target"), scene.camera.target);
            scene.camera.up = ReadVec3(camera->Find("up"), scene.camera.up);
            scene.camera.fovYDegrees = static_cast<float>(camera->Find("fov_y") ? camera->Find("fov_y")->AsNumber(scene.camera.fovYDegrees) : scene.camera.fovYDegrees);
            scene.camera.nearPlane = static_cast<float>(camera->Find("near") ? camera->Find("near")->AsNumber(scene.camera.nearPlane) : scene.camera.nearPlane);
            scene.camera.farPlane = static_cast<float>(camera->Find("far") ? camera->Find("far")->AsNumber(scene.camera.farPlane) : scene.camera.farPlane);
        }
    }

    if (const JsonValue* materials = root.Find("materials"))
    {
        if (materials->IsArray())
        {
            for (const JsonValue& materialValue : materials->AsArray())
            {
                Material material;
                if (materialValue.IsObject())
                {
                    material.baseColor = ReadVec3(materialValue.Find("baseColor"), material.baseColor);
                    material.metallic = static_cast<float>(materialValue.Find("metallic") ? materialValue.Find("metallic")->AsNumber(material.metallic) : material.metallic);
                    material.roughness = static_cast<float>(materialValue.Find("roughness") ? materialValue.Find("roughness")->AsNumber(material.roughness) : material.roughness);

                    const JsonValue* texturePath = materialValue.Find("baseColorTexture");
                    if (texturePath && texturePath->IsString())
                    {
                        Texture2D texture;
                        if (texture.LoadFromFile(ResolveRelativePath(baseDir, texturePath->AsString())))
                        {
                            material.baseColorTexture = static_cast<int>(scene.textures.size());
                            scene.textures.push_back(std::move(texture));
                        }
                    }
                    texturePath = materialValue.Find("normalTexture");
                    if (texturePath && texturePath->IsString())
                    {
                        Texture2D texture;
                        if (texture.LoadFromFile(ResolveRelativePath(baseDir, texturePath->AsString())))
                        {
                            material.normalTexture = static_cast<int>(scene.textures.size());
                            scene.textures.push_back(std::move(texture));
                        }
                    }
                    texturePath = materialValue.Find("metallicRoughnessTexture");
                    if (texturePath && texturePath->IsString())
                    {
                        Texture2D texture;
                        if (texture.LoadFromFile(ResolveRelativePath(baseDir, texturePath->AsString())))
                        {
                            material.metallicRoughnessTexture = static_cast<int>(scene.textures.size());
                            scene.textures.push_back(std::move(texture));
                        }
                    }

                    // Separate grayscale metallic / roughness maps (common in
                    // PBR texture packs) are packed into one metallic-roughness
                    // texture matching the glTF convention.
                    const JsonValue* metallicPath = materialValue.Find("metallicTexture");
                    const JsonValue* roughnessPath = materialValue.Find("roughnessTexture");
                    if ((metallicPath && metallicPath->IsString()) || (roughnessPath && roughnessPath->IsString()))
                    {
                        const std::string resolvedMetallic = (metallicPath && metallicPath->IsString())
                            ? ResolveRelativePath(baseDir, metallicPath->AsString())
                            : std::string();
                        const std::string resolvedRoughness = (roughnessPath && roughnessPath->IsString())
                            ? ResolveRelativePath(baseDir, roughnessPath->AsString())
                            : std::string();

                        Texture2D packed;
                        if (packed.LoadPackedMetallicRoughness(resolvedMetallic, resolvedRoughness))
                        {
                            material.metallicRoughnessTexture = static_cast<int>(scene.textures.size());
                            scene.textures.push_back(std::move(packed));
                        }
                    }
                }
                scene.materials.push_back(std::move(material));
            }
        }
    }

    if (scene.materials.empty())
    {
        scene.materials.push_back(Material{});
    }

    if (const JsonValue* lights = root.Find("lights"))
    {
        if (lights->IsArray())
        {
            for (const JsonValue& lightValue : lights->AsArray())
            {
                DirectionalLight light;
                if (lightValue.IsObject())
                {
                    light.direction = ReadVec3(lightValue.Find("direction"), light.direction);
                    light.color = ReadVec3(lightValue.Find("color"), light.color);
                    light.intensity = static_cast<float>(lightValue.Find("intensity") ? lightValue.Find("intensity")->AsNumber(light.intensity) : light.intensity);
                }
                scene.lights.push_back(light);
            }
        }
    }

    if (scene.lights.empty())
    {
        scene.lights.push_back(DirectionalLight{});
    }

    if (const JsonValue* meshes = root.Find("meshes"))
    {
        if (meshes->IsArray())
        {
            for (const JsonValue& meshValue : meshes->AsArray())
            {
                if (!meshValue.IsObject())
                {
                    continue;
                }

                Mesh mesh;
                const std::string objPath = meshValue.Find("obj") ? meshValue.Find("obj")->AsString() : std::string();
                if (!objPath.empty())
                {
                    if (!LoadObj(ResolveRelativePath(baseDir, objPath), mesh))
                    {
                        continue;
                    }
                }
                else
                {
                    mesh.vertices = {
                        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
                        {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
                        {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
                    };
                    mesh.triangles.push_back({0, 1, 2});
                }

                mesh.materialId = static_cast<int>(meshValue.Find("material") ? meshValue.Find("material")->AsNumber(0.0) : 0.0);
                if (mesh.materialId < 0 || mesh.materialId >= static_cast<int>(scene.materials.size()))
                {
                    mesh.materialId = 0;
                }
                mesh.transform = ComposeTransform(meshValue.Find("transform"));
                scene.meshes.push_back(std::move(mesh));
            }
        }
    }

    if (scene.meshes.empty())
    {
        Mesh mesh;
        mesh.materialId = 0;
        mesh.vertices = {
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
            {{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
            {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.5f, 1.0f}},
        };
        mesh.triangles.push_back({0, 1, 2});
        scene.meshes.push_back(mesh);
    }

    if (const JsonValue* spheres = root.Find("spheres"))
    {
        if (spheres->IsArray())
        {
            for (const JsonValue& sphereValue : spheres->AsArray())
            {
                if (!sphereValue.IsObject())
                {
                    continue;
                }

                Sphere sphere;
                sphere.center = ReadVec3(sphereValue.Find("center"), sphere.center);
                sphere.radius = static_cast<float>(sphereValue.Find("radius") ? sphereValue.Find("radius")->AsNumber(sphere.radius) : sphere.radius);
                sphere.materialId = static_cast<int>(sphereValue.Find("material") ? sphereValue.Find("material")->AsNumber(0.0) : 0.0);
                if (sphere.materialId < 0 || sphere.materialId >= static_cast<int>(scene.materials.size()))
                {
                    sphere.materialId = 0;
                }
                scene.spheres.push_back(sphere);
            }
        }
    }

    return true;
}
} // namespace hr
