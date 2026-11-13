#pragma once

#include <react/renderer/graphics/Float.h>

#include <Evas.h>

#include <algorithm>
#include <cmath>

namespace rntizen {

/// A point on the canvas, in device pixels.
struct PixelPoint {
  Evas_Coord x{0};
  Evas_Coord y{0};

  bool operator==(const PixelPoint &) const = default;
};

/// A rectangle on the canvas, in device pixels.
struct PixelRect {
  Evas_Coord x{0};
  Evas_Coord y{0};
  Evas_Coord width{0};
  Evas_Coord height{0};

  bool operator==(const PixelRect &) const = default;
};

/// `points` layout points in device pixels, rounded to the nearest pixel.
inline Evas_Coord toPixels(facebook::react::Float points, facebook::react::Float scale) {
  return static_cast<Evas_Coord>(std::lround(points * scale));
}

/// `rect` grown by the given amount on each side; a negative amount shrinks it.
inline PixelRect inflated(const PixelRect &rect, Evas_Coord left, Evas_Coord top, Evas_Coord right, Evas_Coord bottom) {
  return {.x = rect.x - left, .y = rect.y - top, .width = rect.width + left + right, .height = rect.height + top + bottom};
}

/// What `a` and `b` share, or an empty rectangle.
inline PixelRect intersection(const PixelRect &a, const PixelRect &b) {
  const auto left = std::max(a.x, b.x);
  const auto top = std::max(a.y, b.y);
  const auto right = std::min(a.x + a.width, b.x + b.width);
  const auto bottom = std::min(a.y + a.height, b.y + b.height);
  if (right <= left || bottom <= top) return {};
  return {.x = left, .y = top, .width = right - left, .height = bottom - top};
}

/// Whether `a` and `b` share at least one pixel.
inline bool overlap(const PixelRect &a, const PixelRect &b) {
  return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

inline void setGeometry(Evas_Object *object, const PixelRect &rect) {
  evas_object_geometry_set(object, rect.x, rect.y, rect.width, rect.height);
}

} // namespace rntizen
