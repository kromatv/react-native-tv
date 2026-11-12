#pragma once

#include <react/renderer/graphics/Float.h>

#include <cmath>

namespace rnwebos {

/// A point in the window, in device pixels.
struct PixelPoint {
  int x{0};
  int y{0};

  bool operator==(const PixelPoint &) const = default;
};

/// A size in device pixels.
struct PixelSize {
  int width{0};
  int height{0};

  bool operator==(const PixelSize &) const = default;
};

/// A rectangle in the window, in device pixels.
struct PixelRect {
  int x{0};
  int y{0};
  int width{0};
  int height{0};

  bool operator==(const PixelRect &) const = default;
};

/// `points` layout points in device pixels, rounded to the nearest pixel.
inline int toPixels(facebook::react::Float points, facebook::react::Float scale) {
  return static_cast<int>(std::lround(points * scale));
}

/// `rect` grown by the given amount on each side; a negative amount shrinks it.
inline PixelRect inflated(const PixelRect &rect, int left, int top, int right, int bottom) {
  return {.x = rect.x - left, .y = rect.y - top, .width = rect.width + left + right, .height = rect.height + top + bottom};
}

} // namespace rnwebos
