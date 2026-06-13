#pragma once

#include <cstddef>
#include <vector>

namespace hr
{
template <typename T>
class Image
{
public:
    Image() = default;

    Image(int width, int height, const T& value = T())
    {
        Resize(width, height, value);
    }

    void Resize(int width, int height, const T& value = T())
    {
        width_ = width;
        height_ = height;
        pixels_.assign(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), value);
    }

    int Width() const { return width_; }
    int Height() const { return height_; }

    bool Empty() const { return pixels_.empty(); }

    T& At(int x, int y)
    {
        return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
    }

    const T& At(int x, int y) const
    {
        return pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
    }

    std::vector<T>& Data() { return pixels_; }
    const std::vector<T>& Data() const { return pixels_; }

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<T> pixels_;
};
} // namespace hr
