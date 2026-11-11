#include "GradientTable.h"

#include <algorithm>
#include <utility>

namespace rntizen {

namespace {

constexpr float kStepsPerPixel = 16;
constexpr std::size_t kMaxEntries = 1 << 16;

} // namespace

GradientTable::GradientTable(
    const GradientGeometry &geometry,
    const GradientRamp &ramp,
    int patchWidth,
    int patchHeight,
    float scaleX,
    float scaleY) {
  const auto pixelSlope = geometry.slope() / std::min(scaleX, scaleY);
  if (pixelSlope <= 0) return;
  const auto lastX = (static_cast<float>(patchWidth) - 0.5f) / scaleX;
  const auto lastY = (static_cast<float>(patchHeight) - 0.5f) / scaleY;
  const auto firstX = 0.5f / scaleX;
  const auto firstY = 0.5f / scaleY;
  float low = 0;
  float high = 0;
  bool seeded = false;
  for (const auto &[x, y] : {std::pair{firstX, firstY}, {lastX, firstY}, {firstX, lastY}, {lastX, lastY}}) {
    const auto t = geometry.at(x, y);
    low = seeded ? std::min(low, t) : t;
    high = seeded ? std::max(high, t) : t;
    seeded = true;
  }
  low = std::min(low, 0.0f);
  const auto stepsPerUnit = kStepsPerPixel / pixelSlope;
  const auto count = static_cast<std::size_t>((high - low) * stepsPerUnit) + 2;
  if (count > kMaxEntries) return;
  start_ = low;
  stepsPerUnit_ = stepsPerUnit;
  pixels_.resize(count);
  std::size_t segment = 0;
  for (std::size_t i = 0; i < count; ++i) pixels_[i] = pixelOf(ramp.at(low + static_cast<float>(i) / stepsPerUnit, segment));
}

bool GradientTable::empty() const {
  return pixels_.empty();
}

std::uint32_t GradientTable::at(float t) const {
  const auto index = static_cast<long>((t - start_) * stepsPerUnit_ + 0.5f);
  return pixels_[static_cast<std::size_t>(std::clamp(index, 0L, static_cast<long>(pixels_.size()) - 1))];
}

} // namespace rntizen
