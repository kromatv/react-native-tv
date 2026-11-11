#include "SvgGradient.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rntizen::svg {

namespace {

constexpr double kTiny = 1e-9;

std::vector<GradientStop> withOpacity(const std::vector<GradientStop> &stops, double opacity) {
  std::vector<GradientStop> faded = stops;
  for (auto &stop : faded) stop.color.alpha *= opacity;
  return faded;
}

Affine gradientSpace(const GradientDefinition &definition, const Bounds &bounds) {
  if (definition.userSpace) return definition.transform;
  return Affine::translation(bounds.x, bounds.y) * Affine::scaling(bounds.width, bounds.height) * definition.transform;
}

double resolveX(const Length &length, const GradientDefinition &definition, double viewportWidth) {
  return length.resolve(definition.userSpace ? viewportWidth : 1);
}

double resolveY(const Length &length, const GradientDefinition &definition, double viewportHeight) {
  return length.resolve(definition.userSpace ? viewportHeight : 1);
}

ResolvedPaint linear(const GradientDefinition &d, const Affine &m, double width, double height, std::vector<GradientStop> stops) {
  const Point from{.x = resolveX(d.x1, d, width), .y = resolveY(d.y1, d, height)};
  const Point to{.x = resolveX(d.x2, d, width), .y = resolveY(d.y2, d, height)};
  const auto dx = to.x - from.x;
  const auto dy = to.y - from.y;
  const auto lengthSquared = dx * dx + dy * dy;
  const auto determinant = m.a * m.d - m.b * m.c;
  if (lengthSquared < kTiny || std::abs(determinant) < kTiny) return stops.back().color;
  const auto wx = (m.d * dx - m.b * dy) / determinant / lengthSquared;
  const auto wy = (m.a * dy - m.c * dx) / determinant / lengthSquared;
  const auto reach = wx * wx + wy * wy;
  const auto start = m.apply(from);
  return GradientPaint{
      .start = start,
      .end = {.x = start.x + wx / reach, .y = start.y + wy / reach},
      .stops = std::move(stops),
  };
}

ResolvedPaint radial(const GradientDefinition &d, const Affine &m, double width, double height, std::vector<GradientStop> stops) {
  const auto diagonal = d.userSpace ? std::sqrt((width * width + height * height) / 2) : 1;
  const auto radius = std::sqrt(std::max(0.0, d.rx.resolve(diagonal) * d.ry.resolve(diagonal)));
  const auto scale = std::sqrt(std::abs(m.a * m.d - m.b * m.c));
  if (radius * scale < kTiny) return stops.back().color;
  return GradientPaint{
      .radial = true,
      .center = m.apply({.x = resolveX(d.cx, d, width), .y = resolveY(d.cy, d, height)}),
      .focal = m.apply({.x = resolveX(d.fx, d, width), .y = resolveY(d.fy, d, height)}),
      .radius = radius * scale,
      .stops = std::move(stops),
  };
}

} // namespace

std::vector<GradientStop> gradientStops(const std::vector<float> &gradient) {
  std::vector<GradientStop> stops;
  double last = 0;
  for (std::size_t i = 0; i + 1 < gradient.size(); i += 2) {
    const auto offset = std::clamp(static_cast<double>(gradient[i]), last, 1.0);
    const auto argb = static_cast<std::uint32_t>(static_cast<std::int64_t>(std::llround(gradient[i + 1])));
    stops.push_back({.offset = offset, .color = rgbaFromArgb(argb)});
    last = offset;
  }
  return stops;
}

ResolvedPaint resolveGradient(
    const GradientDefinition &definition,
    const Bounds &bounds,
    const Affine &toPixels,
    double viewportWidth,
    double viewportHeight,
    double opacity) {
  if (definition.stops.empty()) return {};
  auto stops = withOpacity(definition.stops, opacity);
  if (stops.size() == 1) return stops.front().color;
  if (!definition.userSpace && (bounds.width <= 0 || bounds.height <= 0)) return {};
  const auto m = toPixels * gradientSpace(definition, bounds);
  if (definition.radial) return radial(definition, m, viewportWidth, viewportHeight, std::move(stops));
  return linear(definition, m, viewportWidth, viewportHeight, std::move(stops));
}

} // namespace rntizen::svg
