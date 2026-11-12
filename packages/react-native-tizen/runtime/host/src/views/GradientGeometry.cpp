#include "GradientGeometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>
#include <optional>

namespace rntizen {

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

} // namespace

GradientGeometry::GradientGeometry(
    const facebook::react::BackgroundImage &image,
    float width,
    float height,
    float pointScale)
    : width_(width), height_(height), pointScale_(pointScale) {
  if (const auto *linear = std::get_if<facebook::react::LinearGradient>(&image)) {
    layOutLinear(*linear);
  } else {
    layOutRadial(std::get<facebook::react::RadialGradient>(image));
  }
}

void GradientGeometry::layOutLinear(const facebook::react::LinearGradient &gradient) {
  auto angle = std::fmod(angleOf(gradient.direction, width_, height_), 360.0f);
  if (angle < 0) angle += 360;
  const auto radians = angle * std::numbers::pi_v<float> / 180;
  sin_ = std::sin(radians);
  cos_ = std::cos(radians);
  length_ = std::abs(width_ * sin_) + std::abs(height_ * cos_);
  if (near(angle, 0) || near(angle, 180) || near(angle, 360)) {
    sin_ = 0;
    cos_ = near(angle, 180) ? -1 : 1;
    length_ = height_;
    axis_ = Axis::Vertical;
  } else if (near(angle, 90) || near(angle, 270)) {
    sin_ = near(angle, 90) ? 1 : -1;
    cos_ = 0;
    length_ = width_;
    axis_ = Axis::Horizontal;
  }
}

void GradientGeometry::layOutRadial(const facebook::react::RadialGradient &gradient) {
  radial_ = true;
  const auto &position = gradient.position;
  centerX_ = centerOn(position.left, position.right, width_, pointScale_);
  centerY_ = centerOn(position.top, position.bottom, height_, pointScale_);
  const bool circle = gradient.shape == RadialGradientShape::Circle;
  const auto *size = std::get_if<facebook::react::RadialGradientSize::Dimensions>(&gradient.size.value);
  const auto radii = size != nullptr
      ? explicitRadii(*size, circle, width_, height_, pointScale_)
      : keywordRadii(std::get<SizeKeyword>(gradient.size.value), circle, centerX_, centerY_, width_, height_);
  radiusX_ = std::max(radii.x, kSmallestRadius);
  radiusY_ = std::max(radii.y, kSmallestRadius);
  length_ = radiusX_;
}

float GradientGeometry::lineLength() const {
  return length_;
}

GradientGeometry::Axis GradientGeometry::axis() const {
  return axis_;
}

float GradientGeometry::at(float x, float y) const {
  if (radial_) return std::hypot((x - centerX_) / radiusX_, (y - centerY_) / radiusY_);
  if (length_ <= 0) return 0;
  return ((x - width_ / 2) * sin_ - (y - height_ / 2) * cos_) / length_ + 0.5f;
}

void GradientGeometry::row(float x0, float step, float y, int count, float *out) const {
  if (radial_) {
    const auto dy = (y - centerY_) / radiusY_;
    const auto dy2 = dy * dy;
    for (int i = 0; i < count; ++i) {
      const auto dx = (x0 + static_cast<float>(i) * step - centerX_) / radiusX_;
      out[i] = std::sqrt(dx * dx + dy2);
    }
    return;
  }
  if (length_ <= 0) {
    std::fill(out, out + count, 0.0f);
    return;
  }
  const auto base = (-(y - height_ / 2) * cos_) / length_ + 0.5f;
  const auto perX = sin_ / length_;
  for (int i = 0; i < count; ++i) out[i] = base + (x0 + static_cast<float>(i) * step - width_ / 2) * perX;
}

float GradientGeometry::slope() const {
  if (radial_) return 1 / std::min(radiusX_, radiusY_);
  return length_ > 0 ? (std::abs(sin_) + std::abs(cos_)) / length_ : 0;
}

} // namespace rntizen
