#pragma once

#include <react/renderer/graphics/BackgroundImage.h>

#include "include/core/SkPoint.h"

namespace rnwebos {

/// Where a CSS gradient lies in its box, as CSS Images 3 lays out
/// `linear-gradient` and `radial-gradient`, in pixels from the box's corner.
struct GradientGeometry {
  bool radial{false};
  /// A linear gradient's line: 0% is at `start`, 100% at `end`.
  SkPoint start{};
  SkPoint end{};
  /// A radial gradient's ending shape: 0% is at `center`, 100% on the
  /// ellipse of radii `radiusX` and `radiusY` around it.
  SkPoint center{};
  float radiusX{1};
  float radiusY{1};
  /// The line's length in pixels, which a length color stop is measured against.
  float lineLength{0};
};

/// Lays `image` out in a box of `width` x `height` pixels at `pointScale`
/// pixels per point.
GradientGeometry layOutGradient(const facebook::react::BackgroundImage &image, float width, float height, float pointScale);

} // namespace rnwebos
