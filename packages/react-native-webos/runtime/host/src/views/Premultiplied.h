#pragma once

#include <array>
#include <cstdint>

namespace rntizen {

/// A premultiplied RGBA color, each channel from 0 to 1.
using Premultiplied = std::array<float, 4>;

/// `color` as one premultiplied ARGB32 pixel, as Evas images store them.
std::uint32_t pixelOf(const Premultiplied &color);

/// White at `alpha`, premultiplied: the pixel of a patch its object's color tints.
std::uint32_t alphaPixel(float alpha);

} // namespace rntizen
