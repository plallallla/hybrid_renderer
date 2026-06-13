#pragma once

#include "../core/image.h"
#include "../core/math.h"
#include "../third_party/stb_image.h"

#include <cmath>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

namespace hr
{
class Texture2D
{
public:
    bool LoadFromFile(const std::string& path)
    {
        const std::string lower = ToLowerExtension(path);
        if (lower == ".ppm")
        {
            return LoadPPM(path);
        }
        // png / jpg / jpeg / tga / bmp etc. are decoded by stb_image.
        return LoadSTB(path);
    }

    // Loads two separate grayscale metallic / roughness maps and packs them
    // into a single texture matching the glTF metallic-roughness convention
    // sampled by MaterialEvaluator (G = roughness, B = metallic). Either path
    // may be empty/missing, in which case that channel uses a sensible default
    // (roughness 1.0, metallic 0.0).
    bool LoadPackedMetallicRoughness(const std::string& metallicPath, const std::string& roughnessPath)
    {
        Texture2D metalTex;
        Texture2D roughTex;
        const bool hasMetal = !metallicPath.empty() && metalTex.LoadFromFile(metallicPath);
        const bool hasRough = !roughnessPath.empty() && roughTex.LoadFromFile(roughnessPath);
        if (!hasMetal && !hasRough)
        {
            return false;
        }

        const Image<Vec3>& sizeRef = hasRough ? roughTex.image : metalTex.image;
        const int width = sizeRef.Width();
        const int height = sizeRef.Height();
        image.Resize(width, height, {0.0f, 0.0f, 0.0f});

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                float roughness = 1.0f;
                if (hasRough)
                {
                    roughness = roughTex.image.At(x, y).x;
                }

                float metallic = 0.0f;
                if (hasMetal)
                {
                    const int mx = (metalTex.image.Width() == width) ? x : x * metalTex.image.Width() / width;
                    const int my = (metalTex.image.Height() == height) ? y : y * metalTex.image.Height() / height;
                    metallic = metalTex.image.At(mx, my).x;
                }

                image.At(x, y) = {0.0f, roughness, metallic};
            }
        }
        return true;
    }

    bool LoadSTB(const std::string& path)
    {
        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 3);
        if (data == nullptr || width <= 0 || height <= 0)
        {
            if (data != nullptr)
            {
                stbi_image_free(data);
            }
            return false;
        }

        image.Resize(width, height, {0.0f, 0.0f, 0.0f});
        const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        for (std::size_t i = 0; i < pixelCount; ++i)
        {
            image.Data()[i] = {
                static_cast<float>(data[i * 3 + 0]) / 255.0f,
                static_cast<float>(data[i * 3 + 1]) / 255.0f,
                static_cast<float>(data[i * 3 + 2]) / 255.0f,
            };
        }
        stbi_image_free(data);
        return true;
    }

    bool LoadPPM(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }

        std::string magic;
        file >> magic;
        if (magic != "P3" && magic != "P6")
        {
            return false;
        }

        auto ReadToken = [&file]() -> std::string {
            std::string token;
            while (file >> token)
            {
                if (!token.empty() && token[0] == '#')
                {
                    std::string discard;
                    std::getline(file, discard);
                    continue;
                }
                return token;
            }
            return {};
        };

        const int width = std::stoi(ReadToken());
        const int height = std::stoi(ReadToken());
        const int maxValue = std::stoi(ReadToken());
        if (width <= 0 || height <= 0 || maxValue <= 0)
        {
            return false;
        }

        image.Resize(width, height, {0.0f, 0.0f, 0.0f});

        if (magic == "P3")
        {
            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const float r = static_cast<float>(std::stoi(ReadToken())) / static_cast<float>(maxValue);
                    const float g = static_cast<float>(std::stoi(ReadToken())) / static_cast<float>(maxValue);
                    const float b = static_cast<float>(std::stoi(ReadToken())) / static_cast<float>(maxValue);
                    image.At(x, y) = {r, g, b};
                }
            }
            return true;
        }

        file >> std::ws;
        const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
        std::string buffer(pixelCount * 3, '\0');
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        if (file.gcount() != static_cast<std::streamsize>(buffer.size()))
        {
            return false;
        }

        for (size_t i = 0; i < pixelCount; ++i)
        {
            const unsigned char r = static_cast<unsigned char>(buffer[i * 3 + 0]);
            const unsigned char g = static_cast<unsigned char>(buffer[i * 3 + 1]);
            const unsigned char b = static_cast<unsigned char>(buffer[i * 3 + 2]);
            image.Data()[i] = {
                static_cast<float>(r) / static_cast<float>(maxValue),
                static_cast<float>(g) / static_cast<float>(maxValue),
                static_cast<float>(b) / static_cast<float>(maxValue),
            };
        }
        return true;
    }

    bool LoadTGA(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }

        std::uint8_t header[18] = {};
        file.read(reinterpret_cast<char*>(header), sizeof(header));
        if (file.gcount() != static_cast<std::streamsize>(sizeof(header)))
        {
            return false;
        }

        const std::uint8_t idLength = header[0];
        const std::uint8_t colorMapType = header[1];
        const std::uint8_t imageType = header[2];
        const std::uint16_t width = static_cast<std::uint16_t>(header[12] | (header[13] << 8));
        const std::uint16_t height = static_cast<std::uint16_t>(header[14] | (header[15] << 8));
        const std::uint8_t bpp = header[16];
        const std::uint8_t descriptor = header[17];

        if (colorMapType != 0 || (imageType != 2 && imageType != 3) || width == 0 || height == 0)
        {
            return false;
        }

        if (bpp != 24 && bpp != 32)
        {
            return false;
        }

        if (idLength > 0)
        {
            file.ignore(idLength);
        }

        const int channels = bpp / 8;
        const bool originTop = (descriptor & 0x20U) != 0;
        const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        std::vector<std::uint8_t> buffer(pixelCount * static_cast<std::size_t>(channels));
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        if (file.gcount() != static_cast<std::streamsize>(buffer.size()))
        {
            return false;
        }

        image.Resize(static_cast<int>(width), static_cast<int>(height), {0.0f, 0.0f, 0.0f});
        for (std::size_t y = 0; y < height; ++y)
        {
            const std::size_t srcY = originTop ? y : (height - 1U - y);
            for (std::size_t x = 0; x < width; ++x)
            {
                const std::size_t index = (srcY * width + x) * static_cast<std::size_t>(channels);
                const float b = static_cast<float>(buffer[index + 0]) / 255.0f;
                const float g = static_cast<float>(buffer[index + 1]) / 255.0f;
                const float r = static_cast<float>(buffer[index + 2]) / 255.0f;
                image.At(static_cast<int>(x), static_cast<int>(y)) = {r, g, b};
            }
        }
        return true;
    }

    Vec3 Sample(float u, float v) const
    {
        if (image.Empty())
        {
            return {1.0f, 1.0f, 1.0f};
        }

        u -= std::floor(u);
        v -= std::floor(v);
        const int x = static_cast<int>(u * static_cast<float>(image.Width() - 1));
        const int y = static_cast<int>((1.0f - v) * static_cast<float>(image.Height() - 1));
        return image.At(Clamp(x, 0, image.Width() - 1), Clamp(y, 0, image.Height() - 1));
    }

    const Image<Vec3>& ImageData() const { return image; }

private:
    static std::string ToLowerExtension(const std::string& path)
    {
        const std::size_t dot = path.find_last_of('.');
        if (dot == std::string::npos)
        {
            return {};
        }

        std::string ext = path.substr(dot);
        for (char& c : ext)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return ext;
    }

    Image<Vec3> image;
};
} // namespace hr
