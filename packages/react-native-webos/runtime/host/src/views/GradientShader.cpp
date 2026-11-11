#include "GradientShader.h"

#include "GradientGeometry.h"
#include "GradientRamp.h"

#include "include/core/SkMatrix.h"
#include "include/effects/SkGradient.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::BackgroundImage;
using facebook::react::LinearGradient;
using facebook::react::RadialGradient;

constexpr std::size_t kMostCachedShaders = 128;
constexpr float kShortestReach = 1e-6f;

void mix(std::size_t &seed, std::size_t value) {
  seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
}

void mix(std::size_t &seed, float value) {
  mix(seed, std::hash<float>{}(value));
}

void mix(std::size_t &seed, const std::optional<facebook::react::ValueUnit> &value) {
  mix(seed, value ? value->value : -1.0f);
  mix(seed, static_cast<std::size_t>(value ? value->unit : facebook::react::UnitType::Undefined));
}

void mix(std::size_t &seed, const std::vector<facebook::react::ColorStop> &stops) {
  for (const auto &stop : stops) {
    mix(seed, std::hash<facebook::react::SharedColor>{}(stop.color));
    mix(seed, std::optional{stop.position});
  }
}

void mix(std::size_t &seed, const LinearGradient &linear) {
  mix(seed, linear.direction.index());
  if (const auto *angle = std::get_if<facebook::react::Float>(&linear.direction)) {
    mix(seed, static_cast<float>(*angle));
  } else {
    mix(seed, static_cast<std::size_t>(std::get<facebook::react::GradientKeyword>(linear.direction)));
  }
  mix(seed, linear.colorStops);
}

void mix(std::size_t &seed, const RadialGradient &radial) {
  mix(seed, static_cast<std::size_t>(radial.shape) + 7);
  if (const auto *size = std::get_if<facebook::react::RadialGradientSize::Dimensions>(&radial.size.value)) {
    mix(seed, std::optional{size->x});
    mix(seed, std::optional{size->y});
  } else {
    mix(seed, static_cast<std::size_t>(std::get<facebook::react::RadialGradientSize::SizeKeyword>(radial.size.value)));
  }
  for (const auto *side : {&radial.position.top, &radial.position.left, &radial.position.right, &radial.position.bottom}) {
    mix(seed, *side);
  }
  mix(seed, radial.colorStops);
}

struct CachedShader {
  std::size_t hash;
  BackgroundImage image;
  std::array<float, 3> box;
  sk_sp<SkShader> shader;
  std::uint64_t lastUse;
};

const std::vector<facebook::react::ColorStop> &stopsOf(const BackgroundImage &image) {
  return std::visit([](const auto &gradient) -> const auto & { return gradient.colorStops; }, image);
}

SkGradient gradientOf(const GradientRamp &ramp) {
  return {
      {ramp.colors(), ramp.positions(), SkTileMode::kClamp},
      {.fInPremul = SkGradient::Interpolation::InPremul::kYes},
  };
}

} // namespace

sk_sp<SkShader> makeGradientShader(const BackgroundImage &image, float width, float height, float pointScale) {
  if (width <= 0 || height <= 0) return nullptr;
  const auto geometry = layOutGradient(image, width, height, pointScale);
  const GradientRamp ramp(stopsOf(image), geometry.lineLength, pointScale);
  if (ramp.empty()) return nullptr;
  if (!geometry.radial) {
    const auto visible = ramp.between(0, 1);
    const std::array<SkPoint, 2> line{geometry.start, geometry.end};
    return SkShaders::LinearGradient(line.data(), gradientOf(visible));
  }
  // Skia's last stop is at the circle's edge, CSS's may lie past the ending
  // shape: the circle grows to the last stop.
  const auto reach = std::max(ramp.positions().back(), kShortestReach);
  const auto visible = ramp.between(0, reach);
  SkMatrix ellipse;
  ellipse.setScale(1, geometry.radiusY / geometry.radiusX, geometry.center.x(), geometry.center.y());
  return SkShaders::RadialGradient(geometry.center, geometry.radiusX * reach, gradientOf(visible), &ellipse);
}

const sk_sp<SkShader> &gradientShader(const BackgroundImage &image, float width, float height, float pointScale) {
  static std::vector<CachedShader> cache;
  static std::uint64_t uses = 0;
  const std::array<float, 3> box{width, height, pointScale};
  std::size_t hash = image.index();
  std::visit([&hash](const auto &gradient) { mix(hash, gradient); }, image);
  for (const auto side : box) mix(hash, side);
  for (auto &entry : cache) {
    if (entry.hash != hash || entry.box != box || entry.image != image) continue;
    entry.lastUse = ++uses;
    return entry.shader;
  }
  if (cache.size() >= kMostCachedShaders) {
    cache.erase(std::ranges::min_element(cache, {}, &CachedShader::lastUse));
  }
  cache.push_back({hash, image, box, makeGradientShader(image, width, height, pointScale), ++uses});
  return cache.back().shader;
}

} // namespace rnwebos
