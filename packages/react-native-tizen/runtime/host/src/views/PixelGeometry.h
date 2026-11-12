#pragma once

#include <react/renderer/graphics/Float.h>

#include <Evas.h>

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

inline void setGeometry(Evas_Object *object, const PixelRect &rect) {
  evas_object_geometry_set(object, rect.x, rect.y, rect.width, rect.height);
}

} // namespace rntizen
