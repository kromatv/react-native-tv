#include "Premultiplied.h"

#include <algorithm>

namespace rntizen {

namespace {

std::uint32_t byteOf(float unit) {
  return static_cast<std::uint32_t>(static_cast<double>(std::clamp(unit, 0.0f, 1.0f) * 255) + 0.5);
}

} // namespace

std::uint32_t pixelOf(const Premultiplied &color) {
  return byteOf(color[3]) << 24 | byteOf(color[0]) << 16 | byteOf(color[1]) << 8 | byteOf(color[2]);
}

std::uint32_t alphaPixel(float alpha) {
  const auto a = byteOf(alpha);
  return a << 24 | a << 16 | a << 8 | a;
}

} // namespace rntizen
