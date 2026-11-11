#include "SvgGradient.h"

#include "include/core/SkPoint.h"
#include "include/core/SkTileMode.h"
#include "include/effects/SkGradient.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace rnwebos::svg {

namespace {

constexpr float kTiny = 1e-6F;
// SVG 1.1 moves a focal point outside the circle onto it; just inside keeps Skia's cone a disc.
constexpr float kFarthestFocus = 0.99F;

struct Ramp {
  std::vector<SkColor4f> colors;
  std::vector<float> offsets;

  SkGradient gradient() const {
    return {SkGradient::Colors(colors, offsets, SkTileMode::kClamp), SkGradient::Interpolation{}};
  }
};

Ramp rampOf(const std::vector<GradientStop> &stops) {
  Ramp ramp;
  ramp.colors.reserve(stops.size());
  ramp.offsets.reserve(stops.size());
  for (const auto &stop : stops) {
    ramp.colors.push_back(stop.color);
    ramp.offsets.push_back(stop.offset);
  }
  return ramp;
}

SkMatrix gradientSpace(const GradientDefinition &definition, const SkRect &bounds) {
  if (definition.userSpace) return definition.transform;
  auto space = SkMatrix::ScaleTranslate(bounds.width(), bounds.height(), bounds.left(), bounds.top());
  space.preConcat(definition.transform);
  return space;
}

SkPoint resolvePoint(const Length &x, const Length &y, const GradientDefinition &definition, SkSize viewport) {
  if (!definition.userSpace) return {x.resolve(1), y.resolve(1)};
  return {x.resolve(viewport.width()), y.resolve(viewport.height())};
}

GradientPaint linear(const GradientDefinition &definition, const SkMatrix &space, SkSize viewport) {
  const std::array<SkPoint, 2> points{
      resolvePoint(definition.x1, definition.y1, definition, viewport),
      resolvePoint(definition.x2, definition.y2, definition, viewport),
  };
  if (SkPoint::Distance(points[0], points[1]) < kTiny) return definition.stops.back().color;
  return SkShaders::LinearGradient(points.data(), rampOf(definition.stops).gradient(), &space);
}

GradientPaint radial(const GradientDefinition &definition, const SkMatrix &space, SkSize viewport) {
  const float diagonal = definition.userSpace
      ? std::sqrt((viewport.width() * viewport.width() + viewport.height() * viewport.height()) / 2)
      : 1;
  const float rx = definition.rx.resolve(diagonal);
  const float ry = definition.ry.resolve(diagonal);
  if (rx < kTiny || ry < kTiny) return definition.stops.back().color;
  const auto center = resolvePoint(definition.cx, definition.cy, definition, viewport);
  const auto focal = resolvePoint(definition.fx, definition.fy, definition, viewport);
  auto unitCircle = space;
  unitCircle.preTranslate(center.x(), center.y());
  unitCircle.preScale(rx, ry);
  SkPoint focus{(focal.x() - center.x()) / rx, (focal.y() - center.y()) / ry};
  const auto ramp = rampOf(definition.stops);
  if (focus.length() < kTiny) return SkShaders::RadialGradient({0, 0}, 1, ramp.gradient(), &unitCircle);
  if (focus.length() > kFarthestFocus) focus.setLength(kFarthestFocus);
  return SkShaders::TwoPointConicalGradient(focus, 0, {0, 0}, 1, ramp.gradient(), &unitCircle);
}

} // namespace

std::vector<GradientStop> gradientStops(const std::vector<float> &gradient) {
  std::vector<GradientStop> stops;
  float last = 0;
  for (std::size_t i = 0; i + 1 < gradient.size(); i += 2) {
    const auto offset = std::clamp(gradient[i], last, 1.0F);
    const auto argb = static_cast<std::uint32_t>(static_cast<std::int64_t>(std::llround(gradient[i + 1])));
    stops.push_back({.offset = offset, .color = colorFromArgb(argb)});
    last = offset;
  }
  return stops;
}

GradientPaint gradientPaint(const GradientDefinition &definition, const SkRect &bounds, SkSize viewport) {
  if (definition.stops.empty()) return {};
  if (definition.stops.size() == 1) return definition.stops.front().color;
  if (!definition.userSpace && (bounds.width() <= 0 || bounds.height() <= 0)) return {};
  const auto space = gradientSpace(definition, bounds);
  if (!space.invert()) return definition.stops.back().color;
  auto paint = definition.radial ? radial(definition, space, viewport) : linear(definition, space, viewport);
  if (const auto *shader = std::get_if<sk_sp<SkShader>>(&paint); shader != nullptr && !*shader) {
    return definition.stops.back().color;
  }
  return paint;
}

} // namespace rnwebos::svg
