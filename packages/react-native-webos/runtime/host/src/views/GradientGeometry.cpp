#include "GradientGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <optional>

namespace rnwebos {

namespace {

using facebook::react::GradientKeyword;
using facebook::react::RadialGradientShape;
using SizeKeyword = facebook::react::RadialGradientSize::SizeKeyword;

constexpr float kAxisTolerance = 1e-3f;
constexpr float kSmallestRadius = 1e-3f;

float degrees(float radians) {
  return radians * 180 / std::numbers::pi_v<float>;
}

float angleOf(const facebook::react::GradientDirection &direction, float width, float height) {
  if (const auto *angle = std::get_if<facebook::react::Float>(&direction)) return *angle;
  const auto corner = degrees(std::atan2(height, width));
  using enum GradientKeyword;
  switch (std::get<GradientKeyword>(direction)) {
    case ToTopRight:
      return corner;
    case ToBottomRight:
      return 180 - corner;
    case ToBottomLeft:
      return 180 + corner;
    case ToTopLeft:
      return 360 - corner;
  }
  return 180;
}

bool near(float value, float target) {
  return std::abs(value - target) < kAxisTolerance;
}

float resolve(const facebook::react::ValueUnit &value, float reference, float pointScale) {
  return value.unit == facebook::react::UnitType::Percent ? value.value * reference / 100 : value.value * pointScale;
}

float centerOn(
    const std::optional<facebook::react::ValueUnit> &start,
    const std::optional<facebook::react::ValueUnit> &end,
    float length,
    float pointScale) {
  if (start) return resolve(*start, length, pointScale);
  if (end) return length - resolve(*end, length, pointScale);
  return length / 2;
}

struct Radii {
  float x;
  float y;
};

struct CornerExtent {
  float distance;
  std::array<float, 2> offset;
};

Radii explicitRadii(
    const facebook::react::RadialGradientSize::Dimensions &size,
    bool circle,
    float width,
    float height,
    float pointScale) {
  const auto x = resolve(size.x, width, pointScale);
  const auto y = size.y ? resolve(size.y, height, pointScale) : x;
  if (!circle) return {.x = x, .y = y};
  const auto radius = std::max(x, y);
  return {.x = radius, .y = radius};
}

CornerExtent cornerExtent(float centerX, float centerY, float width, float height, bool closest) {
  const std::array<std::array<float, 2>, 4> corners{{{0, 0}, {width, 0}, {width, height}, {0, height}}};
  CornerExtent extent{.distance = closest ? std::numeric_limits<float>::infinity() : -1.0f, .offset = {}};
  for (const auto &[x, y] : corners) {
    const auto d = std::hypot(x - centerX, y - centerY);
    if (closest ? d < extent.distance : d > extent.distance) extent = {.distance = d, .offset = {x - centerX, y - centerY}};
  }
  return extent;
}

Radii keywordRadii(SizeKeyword keyword, bool circle, float centerX, float centerY, float width, float height) {
  using enum SizeKeyword;
  const bool closest = keyword == ClosestSide || keyword == ClosestCorner;
  const auto pick = [closest](float a, float b) { return closest ? std::min(a, b) : std::max(a, b); };
  const auto sideX = pick(std::abs(centerX), std::abs(width - centerX));
  const auto sideY = pick(std::abs(centerY), std::abs(height - centerY));
  if (keyword == ClosestSide || keyword == FarthestSide) {
    if (!circle) return {.x = sideX, .y = sideY};
    const auto radius = pick(sideX, sideY);
    return {.x = radius, .y = radius};
  }
  const auto extent = cornerExtent(centerX, centerY, width, height, closest);
  if (circle || sideX <= 0 || sideY <= 0) return {.x = extent.distance, .y = extent.distance};
  const auto ratio = sideX / sideY;
  const auto &[dx, dy] = extent.offset;
  const auto radiusX = std::sqrt(dx * dx + dy * dy * ratio * ratio);
  return {.x = radiusX, .y = radiusX / ratio};
}

GradientGeometry layOutLinear(const facebook::react::LinearGradient &gradient, float width, float height) {
  auto angle = std::fmod(angleOf(gradient.direction, width, height), 360.0f);
  if (angle < 0) angle += 360;
  const auto radians = angle * std::numbers::pi_v<float> / 180;
  auto sin = std::sin(radians);
  auto cos = std::cos(radians);
  auto length = std::abs(width * sin) + std::abs(height * cos);
  if (near(angle, 0) || near(angle, 180) || near(angle, 360)) {
    sin = 0;
    cos = near(angle, 180) ? -1 : 1;
    length = height;
  } else if (near(angle, 90) || near(angle, 270)) {
    sin = near(angle, 90) ? 1 : -1;
    cos = 0;
    length = width;
  }
  const SkPoint middle{width / 2, height / 2};
  const SkVector half{sin * length / 2, -cos * length / 2};
  return {.radial = false, .start = middle - half, .end = middle + half, .lineLength = length};
}

GradientGeometry layOutRadial(const facebook::react::RadialGradient &gradient, float width, float height, float pointScale) {
  const auto &position = gradient.position;
  const auto centerX = centerOn(position.left, position.right, width, pointScale);
  const auto centerY = centerOn(position.top, position.bottom, height, pointScale);
  const bool circle = gradient.shape == RadialGradientShape::Circle;
  const auto *size = std::get_if<facebook::react::RadialGradientSize::Dimensions>(&gradient.size.value);
  const auto radii = size != nullptr
      ? explicitRadii(*size, circle, width, height, pointScale)
      : keywordRadii(std::get<SizeKeyword>(gradient.size.value), circle, centerX, centerY, width, height);
  const auto radiusX = std::max(radii.x, kSmallestRadius);
  return {
      .radial = true,
      .center = {centerX, centerY},
      .radiusX = radiusX,
      .radiusY = std::max(radii.y, kSmallestRadius),
      .lineLength = radiusX,
  };
}

} // namespace

GradientGeometry layOutGradient(const facebook::react::BackgroundImage &image, float width, float height, float pointScale) {
  if (const auto *linear = std::get_if<facebook::react::LinearGradient>(&image)) return layOutLinear(*linear, width, height);
  return layOutRadial(std::get<facebook::react::RadialGradient>(image), width, height, pointScale);
}

} // namespace rnwebos
