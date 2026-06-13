#include "png_writer.h"

#include <array>
#include <cstddef>
#include <cmath>
#include <fstream>
#include <iterator>
#include <vector>

namespace hr
{
namespace
{
uint32_t Crc32(const uint8_t* data, size_t size)
{
    static std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> result{};
        for (uint32_t i = 0; i < 256; ++i)
        {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
            {
                c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
            }
            result[i] = c;
        }
        return result;
    }();

    uint32_t crc = 0xFFFFFFFFU;
    for (size_t i = 0; i < size; ++i)
    {
        crc = table[(crc ^ data[i]) & 0xFFU] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFU;
}

uint32_t Adler32(const std::vector<uint8_t>& data)
{
    constexpr uint32_t mod = 65521U;
    uint32_t a = 1U;
    uint32_t b = 0U;
    for (uint8_t byte : data)
    {
        a = (a + byte) % mod;
        b = (b + a) % mod;
    }
    return (b << 16) | a;
}

void WriteU32(std::vector<uint8_t>& out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
}

void WriteChunk(std::vector<uint8_t>& out, const char type[4], const std::vector<uint8_t>& payload)
{
    WriteU32(out, static_cast<uint32_t>(payload.size()));
    out.push_back(static_cast<uint8_t>(type[0]));
    out.push_back(static_cast<uint8_t>(type[1]));
    out.push_back(static_cast<uint8_t>(type[2]));
    out.push_back(static_cast<uint8_t>(type[3]));
    out.insert(out.end(), payload.begin(), payload.end());

    std::vector<uint8_t> crcInput(4 + payload.size());
    crcInput[0] = static_cast<uint8_t>(type[0]);
    crcInput[1] = static_cast<uint8_t>(type[1]);
    crcInput[2] = static_cast<uint8_t>(type[2]);
    crcInput[3] = static_cast<uint8_t>(type[3]);
    for (size_t i = 0; i < payload.size(); ++i)
    {
        crcInput[4 + i] = payload[i];
    }
    WriteU32(out, Crc32(crcInput.data(), crcInput.size()));
}

uint8_t ToByte(float value)
{
    value = Clamp(value, 0.0f, 1.0f);
    value = std::pow(value, 1.0f / 2.2f);
    return static_cast<uint8_t>(value * 255.0f + 0.5f);
}
} // namespace

bool WritePng(const std::string& path, const Image<Vec3>& image)
{
    if (image.Empty())
    {
        return false;
    }

    const int width = image.Width();
    const int height = image.Height();

    std::vector<uint8_t> png;
    const uint8_t signature[8] = {0x89U, 0x50U, 0x4EU, 0x47U, 0x0DU, 0x0AU, 0x1AU, 0x0AU};
    png.insert(png.end(), signature, signature + 8);

    std::vector<uint8_t> ihdr;
    WriteU32(ihdr, static_cast<uint32_t>(width));
    WriteU32(ihdr, static_cast<uint32_t>(height));
    ihdr.push_back(8U);
    ihdr.push_back(2U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    ihdr.push_back(0U);
    WriteChunk(png, "IHDR", ihdr);

    std::vector<uint8_t> raw;
    raw.reserve(static_cast<size_t>(height) * (static_cast<size_t>(width) * 3U + 1U));
    for (int y = 0; y < height; ++y)
    {
        raw.push_back(0U);
        for (int x = 0; x < width; ++x)
        {
            const Vec3 color = Clamp(image.At(x, y), 0.0f, 1.0f);
            raw.push_back(ToByte(color.x));
            raw.push_back(ToByte(color.y));
            raw.push_back(ToByte(color.z));
        }
    }

    std::vector<uint8_t> zlib;
    zlib.push_back(0x78U);
    zlib.push_back(0x01U);

    size_t offset = 0;
    while (offset < raw.size())
    {
        const size_t remaining = raw.size() - offset;
        const size_t blockSize = remaining > 65535U ? 65535U : remaining;
        const bool finalBlock = (offset + blockSize) == raw.size();
        zlib.push_back(static_cast<uint8_t>(finalBlock ? 0x01U : 0x00U));
        const uint16_t len = static_cast<uint16_t>(blockSize);
        const uint16_t nlen = static_cast<uint16_t>(~len);
        zlib.push_back(static_cast<uint8_t>(len & 0xFFU));
        zlib.push_back(static_cast<uint8_t>((len >> 8) & 0xFFU));
        zlib.push_back(static_cast<uint8_t>(nlen & 0xFFU));
        zlib.push_back(static_cast<uint8_t>((nlen >> 8) & 0xFFU));
        zlib.insert(zlib.end(), raw.begin() + static_cast<std::ptrdiff_t>(offset), raw.begin() + static_cast<std::ptrdiff_t>(offset + blockSize));
        offset += blockSize;
    }

    WriteU32(zlib, Adler32(raw));
    WriteChunk(png, "IDAT", zlib);
    WriteChunk(png, "IEND", {});

    std::ofstream file(path, std::ios::binary);
    if (!file)
    {
        return false;
    }

    file.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return static_cast<bool>(file);
}
} // namespace hr
