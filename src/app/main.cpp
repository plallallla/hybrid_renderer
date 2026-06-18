#include "../asset/scene_loader.h"
#include "../asset/json.h"
#include "render_demo.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace
{
constexpr const char* kDemoEntryPath = "assets/scenes/demo_suite.json";

bool LoadJsonFromDemoPath(const std::string& relativePath, hr::JsonValue& root, std::string& resolvedPath)
{
    for (const std::string& prefix : {"", "../", "../../"})
    {
        const std::string candidate = prefix + relativePath;
        if (hr::LoadJsonFile(candidate, root))
        {
            resolvedPath = candidate;
            return true;
        }
    }
    return false;
}

std::string ResolveSiblingPath(const std::filesystem::path& ownerPath, const std::string& relativePath)
{
    std::filesystem::path path(relativePath);
    if (path.is_absolute())
    {
        return path.string();
    }
    return (ownerPath.parent_path() / path).string();
}

std::string ReadScenePath(const hr::JsonValue& value)
{
    if (value.IsString())
    {
        return value.AsString();
    }
    if (value.IsObject())
    {
        const hr::JsonValue* path = value.Find("path");
        if (path && path->IsString())
        {
            return path->AsString();
        }
    }
    return {};
}

int RunSceneFile(const std::string& scenePath)
{
    hr::JsonValue root;
    if (!hr::LoadJsonFile(scenePath, root))
    {
        std::cerr << "Failed to load scene JSON '" << scenePath << "'." << std::endl;
        return 1;
    }

    hr::Scene scene;
    if (!hr::LoadSceneFromJson(scenePath, scene))
    {
        std::cerr << "Failed to build scene from '" << scenePath << "'." << std::endl;
        return 1;
    }

    std::cout << "\n=== " << scenePath << " ===" << std::endl;
    const hr::DemoConfig config = hr::BuildDemoConfig(root);
    return hr::RunRenderDemo(scene, config);
}
} // namespace

int main()
{
    hr::JsonValue root;
    std::string entryPath;
    if (!LoadJsonFromDemoPath(kDemoEntryPath, root, entryPath))
    {
        std::cerr << "Failed to load demo entry JSON '" << kDemoEntryPath << "'." << std::endl;
        return 1;
    }

    const hr::JsonValue* suite = root.Find("renderSuite");
    if (suite && suite->IsObject())
    {
        const hr::JsonValue* scenes = suite->Find("scenes");
        if (!scenes || !scenes->IsArray() || scenes->AsArray().empty())
        {
            std::cerr << "renderSuite.scenes is empty in '" << entryPath << "'." << std::endl;
            return 1;
        }

        for (const hr::JsonValue& sceneEntry : scenes->AsArray())
        {
            const std::string relativeScenePath = ReadScenePath(sceneEntry);
            if (relativeScenePath.empty())
            {
                std::cerr << "Invalid scene entry in '" << entryPath << "'." << std::endl;
                return 1;
            }
            const int result = RunSceneFile(ResolveSiblingPath(entryPath, relativeScenePath));
            if (result != 0)
            {
                return result;
            }
        }
        return 0;
    }

    return RunSceneFile(entryPath);
}
