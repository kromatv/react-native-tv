#include "GradientRamp.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace rnwebos {

namespace {

using facebook::react::UnitType;

// A hint's curve becomes straight transitions that stay within half a color
// level of it, except in a first sliver too thin to see.
constexpr float kHintTolerance = 0.5f / 255;
constexpr float kThinnestHintStep = 1.0f / 2048;
constexpr std::size_t kMostHintSteps = 32;

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

SkColor4f colorOf(const facebook::react::SharedColor &color) {
  const auto c = facebook::react::colorComponentsFromColor(color);
  return {c.red, c.green, c.blue, std::clamp(c.alpha, 0.0f, 1.0f)};
}

SkColor4f mixed(const SkColor4f &from, const SkColor4f &to, float weight) {
  const auto a = from.premul();
  const auto b = to.premul();
  const SkRGBA4f<kPremul_SkAlphaType> color{
      a.fR + (b.fR - a.fR) * weight,
      a.fG + (b.fG - a.fG) * weight,
      a.fB + (b.fB - a.fB) * weight,
      a.fA + (b.fA - a.fA) * weight,
  };
  return color.unpremul();
}

float chordError(float from, float to, float exponent) {
  const auto low = std::pow(from, exponent);
  const auto high = std::pow(to, exponent);
  float error = 0;
  for (const auto part : {0.25f, 0.5f, 0.75f}) {
    const auto curve = std::pow(std::lerp(from, to, part), exponent);
    error = std::max(error, std::abs(curve - std::lerp(low, high, part)));
  }
  return error;
}

// The fractions of a transition to put stops at so that straight lines
// between them follow `progress ^ exponent`, the widest miss halved first.
std::vector<float> hintSteps(float exponent) {
  std::vector<float> steps{0, 1};
  while (steps.size() < kMostHintSteps) {
    auto worst = steps.end();
    auto worstError = kHintTolerance;
    for (auto step = steps.begin(); step + 1 != steps.end(); ++step) {
      if (*(step + 1) - *step < kThinnestHintStep) continue;
      const auto error = chordError(*step, *(step + 1), exponent);
      if (error > worstError) {
        worst = step;
        worstError = error;
      }
    }
    if (worst == steps.end()) break;
    steps.insert(worst + 1, (*worst + *(worst + 1)) / 2);
  }
  return steps;
}

} // namespace

GradientRamp::GradientRamp(const std::vector<facebook::react::ColorStop> &stops, float lineLength, float pointScale) {
  if (stops.empty()) return;
  const auto positions = fixedPositions(stops, lineLength, pointScale);
  std::optional<float> pendingHint;
  for (std::size_t i = 0; i < stops.size(); ++i) {
    const auto &stop = stops[i];
    if (!stop.color) {
      if (!colors_.empty() && i + 1 < stops.size() && stops[i + 1].color) pendingHint = positions[i];
      continue;
    }
    const auto color = colorOf(stop.color);
    if (pendingHint.has_value()) {
      addHintStops(*pendingHint, color, positions[i]);
      pendingHint.reset();
    }
    colors_.push_back(color);
    positions_.push_back(positions[i]);
  }
}

void GradientRamp::addHintStops(float hintPosition, const SkColor4f &color, float position) {
  const auto from = positions_.back();
  const auto fromColor = colors_.back();
  const auto span = position - from;
  const auto hint = span > 0 ? (hintPosition - from) / span : 0.5f;
  if (hint <= 0) {
    colors_.push_back(color);
    positions_.push_back(from);
    return;
  }
  if (hint >= 1) {
    colors_.push_back(fromColor);
    positions_.push_back(position);
    return;
  }
  const auto exponent = std::log(0.5f) / std::log(hint);
  const auto steps = hintSteps(exponent);
  for (std::size_t step = 1; step + 1 < steps.size(); ++step) {
    colors_.push_back(mixed(fromColor, color, std::pow(steps[step], exponent)));
    positions_.push_back(from + span * steps[step]);
  }
}

bool GradientRamp::empty() const {
  return colors_.empty();
}

const std::vector<SkColor4f> &GradientRamp::colors() const {
  return colors_;
}

const std::vector<float> &GradientRamp::positions() const {
  return positions_;
}

SkColor4f GradientRamp::at(float position, bool after) const {
  const auto [first, last] = std::ranges::equal_range(positions_, position);
  if (first != last) {
    const auto chosen = after ? last - 1 : first;
    return colors_[static_cast<std::size_t>(chosen - positions_.begin())];
  }
  if (first == positions_.begin()) return colors_.front();
  if (first == positions_.end()) return colors_.back();
  const auto upper = static_cast<std::size_t>(first - positions_.begin());
  const auto weight = (position - positions_[upper - 1]) / (positions_[upper] - positions_[upper - 1]);
  return mixed(colors_[upper - 1], colors_[upper], weight);
}

GradientRamp GradientRamp::between(float from, float to) const {
  GradientRamp part;
  if (empty() || !(to > from)) return part;
  const auto add = [&part](const SkColor4f &color, float position) {
    if (!part.colors_.empty() && part.positions_.back() == position && part.colors_.back() == color) return;
    part.colors_.push_back(color);
    part.positions_.push_back(position);
  };
  add(at(from, false), 0);
  add(at(from, true), 0);
  for (std::size_t i = 0; i < positions_.size(); ++i) {
    if (positions_[i] > from && positions_[i] < to) add(colors_[i], (positions_[i] - from) / (to - from));
  }
  add(at(to, false), 1);
  add(at(to, true), 1);
  return part;
}

} // namespace rnwebos
