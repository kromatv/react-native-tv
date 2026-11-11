#pragma once

#include "NinePatch.h"

namespace rntizen {

/// A `width` x `height` rectangle at (`x`, `y`) with rounded `corners`, in
/// pixels.
struct RoundedRect {
  float x{0};
  float y{0};
  float width{0};
  float height{0};
  Corners corners;

  /// Whether the point (`px`, `py`) lies inside.
  bool contains(float px, float py) const;
  /// How much of the pixel at (`pixelX`, `pixelY`) the shape covers, from 0
  /// to 1: exact inside and outside, sampled 4 x 4 where an edge crosses it.
  float coverage(int pixelX, int pixelY) const;
  /// The coverage of the first `count` pixels of row `pixelY`.
  void coverageRow(int pixelY, int count, float *out) const;
};

/// The coverage of the pixel at (`x`, `y`) by a `width` x `height` box with
/// `corners` at the origin, 1 without sampling away from the corners.
float roundedCoverage(const Corners &corners, int width, int height, int x, int y);

} // namespace rntizen
