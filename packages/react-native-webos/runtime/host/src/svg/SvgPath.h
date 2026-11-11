#pragma once

#include "include/core/SkPath.h"
#include "include/core/SkPoint.h"

#include <optional>

namespace rnwebos::svg {

/// An ellipse as SVG winds it, clockwise from its rightmost point, which is
/// where a dash pattern starts. Empty without both radii.
SkPath ellipsePath(SkPoint center, float rx, float ry);
/// A rectangle with SVG's corner rules: a missing radius takes the other's
/// value, and each is clamped to half the side it rounds.
SkPath rectPath(SkPoint origin, float width, float height, std::optional<float> rx, std::optional<float> ry);
SkPath linePath(SkPoint from, SkPoint to);

} // namespace rnwebos::svg
