#include "GradientRamp.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace rntizen {

namespace {

using facebook::react::UnitType;

constexpr float kNoHint = std::numeric_limits<float>::quiet_NaN();

std::optional<float> fraction(const facebook::react::ValueUnit &position, float lineLength, float pointScale) {
  using enum UnitType;
  switch (position.unit) {
    case Percent:
      return position.value / 100;
    case Point:
      return lineLength > 0 ? position.value * pointScale / lineLength : 0;
    case Undefined:
      return std::nullopt;
  }
  return std::nullopt;
}

std::vector<float> fixedPositions(const std::vector<facebook::react::ColorStop> &stops, float lineLength, float pointScale) {
  std::vector<std::optional<float>> positions;
  positions.reserve(stops.size());
  for (const auto &stop : stops) positions.push_back(fraction(stop.position, lineLength, pointScale));
  if (!positions.front().has_value()) positions.front() = 0;
  if (!positions.back().has_value()) positions.back() = 1;
  float largest = *positions.front();
  for (auto &position : positions) {
    if (!position.has_value()) continue;
    position = std::max(*position, largest);
    largest = *position;
  }
  std::size_t last = 0;
  for (std::size_t i = 1; i < positions.size(); ++i) {
    if (!positions[i].has_value()) continue;
    const auto from = *positions[last];
    const auto step = (*positions[i] - from) / static_cast<float>(i - last);
    for (auto j = last + 1; j < i; ++j) positions[j] = from + step * static_cast<float>(j - last);
    last = i;
  }
  std::vector<float> fixed;
  fixed.reserve(positions.size());
  for (const auto &position : positions) fixed.push_back(*position);
  return fixed;
}

Premultiplied premultiplied(const facebook::react::SharedColor &color) {
  const auto c = facebook::react::colorComponentsFromColor(color);
  const auto alpha = std::clamp(c.alpha, 0.0f, 1.0f);
  return {c.red * alpha, c.green * alpha, c.blue * alpha, alpha};
}

float weighting(float progress, float hint) {
  if (std::isnan(hint)) return progress;
  if (hint <= 0) return progress > 0 ? 1 : 0;
  if (hint >= 1) return progress < 1 ? 0 : 1;
  return std::pow(progress, std::log(0.5f) / std::log(hint));
}

} // namespace

GradientRamp::GradientRamp(const std::vector<facebook::react::ColorStop> &stops, float lineLength, float pointScale) {
  if (stops.empty()) return;
  const auto positions = fixedPositions(stops, lineLength, pointScale);
  std::optional<float> pendingHint;
  for (std::size_t i = 0; i < stops.size(); ++i) {
    const auto &stop = stops[i];
    if (!stop.color) {
      if (!stops_.empty() && i + 1 < stops.size() && stops[i + 1].color) pendingHint = positions[i];
      continue;
    }
    Stop next{.position = positions[i], .color = premultiplied(stop.color), .hint = kNoHint};
    if (pendingHint.has_value()) {
      const auto from = stops_.back().position;
      const auto span = next.position - from;
      next.hint = span > 0 ? (*pendingHint - from) / span : kNoHint;
      pendingHint.reset();
    }
    stops_.push_back(next);
  }
}

bool GradientRamp::empty() const {
  return stops_.empty();
}

Premultiplied GradientRamp::at(float t) const {
  std::size_t segment = 0;
  return at(t, segment);
}

Premultiplied GradientRamp::at(float t, std::size_t &segment) const {
  if (t <= stops_.front().position) return stops_.front().color;
  if (t >= stops_.back().position) return stops_.back().color;
  if (const bool inSegment =
          segment > 0 && segment < stops_.size() && stops_[segment - 1].position <= t && t < stops_[segment].position;
      !inSegment) {
    const auto upper = std::ranges::upper_bound(stops_, t, std::ranges::less{}, &Stop::position);
    segment = static_cast<std::size_t>(upper - stops_.begin());
  }
  return between(stops_[segment - 1], stops_[segment], t);
}

Premultiplied GradientRamp::between(const Stop &from, const Stop &to, float t) const {
  const auto weight = weighting((t - from.position) / (to.position - from.position), to.hint);
  Premultiplied color{};
  for (std::size_t c = 0; c < color.size(); ++c) color[c] = from.color[c] + (to.color[c] - from.color[c]) * weight;
  return color;
}

} // namespace rntizen
