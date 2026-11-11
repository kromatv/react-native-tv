#include "SvgPath.h"

#include "include/core/SkPathTypes.h"
#include "include/core/SkRRect.h"
#include "include/core/SkRect.h"

#include <algorithm>

namespace rnwebos::svg {

namespace {

constexpr unsigned kOvalRightmostPoint = 1;
constexpr unsigned kTopEdgeStart = 0;

} // namespace

SkPath ellipsePath(SkPoint center, float rx, float ry) {
  if (rx <= 0 || ry <= 0) return {};
  return SkPath::Oval(
      SkRect::MakeLTRB(center.x() - rx, center.y() - ry, center.x() + rx, center.y() + ry),
      SkPathDirection::kCW,
      kOvalRightmostPoint);
}

SkPath rectPath(SkPoint origin, float width, float height, std::optional<float> rx, std::optional<float> ry) {
  if (width <= 0 || height <= 0) return {};
  const auto rect = SkRect::MakeXYWH(origin.x(), origin.y(), width, height);
  const float radiusX = std::clamp(rx.value_or(ry.value_or(0)), 0.0F, width / 2);
  const float radiusY = std::clamp(ry.value_or(rx.value_or(0)), 0.0F, height / 2);
  if (radiusX == 0 || radiusY == 0) return SkPath::Rect(rect, SkPathDirection::kCW, kTopEdgeStart);
  return SkPath::RRect(SkRRect::MakeRectXY(rect, radiusX, radiusY), SkPathDirection::kCW, kTopEdgeStart);
}

SkPath linePath(SkPoint from, SkPoint to) {
  return SkPath::Line(from, to);
}

} // namespace rnwebos::svg
