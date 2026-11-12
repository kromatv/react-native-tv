#pragma once

#include <react/renderer/graphics/ColorStop.h>

#include "include/core/SkColor.h"

#include <vector>

namespace rnwebos {

/// The colors along a CSS gradient line, as Skia's gradient shaders take
/// them: React Native's color stops fixed up as CSS Images 4 orders them, each
/// transition hint replaced by stops along its curve, and every color meant
/// to be interpolated premultiplied, as browsers do.
class GradientRamp {
 public:
  /// `stops` as React Native parsed them, on a line `lineLength` pixels long,
  /// a length position being in points at `pointScale` pixels each.
  GradientRamp(const std::vector<facebook::react::ColorStop> &stops, float lineLength, float pointScale);

  bool empty() const;
  const std::vector<SkColor4f> &colors() const;
  /// Each color's fraction of the line from its start, never decreasing;
  /// CSS lets them fall before 0 and after 1.
  const std::vector<float> &positions() const;

  /// The part of the ramp from `from` to `to` of the line, the colors beyond
  /// its ends held, its positions as fractions of that part: what Skia
  /// wants, which takes positions from 0 to 1 only.
  GradientRamp between(float from, float to) const;

 private:
  GradientRamp() = default;
  void addHintStops(float hintPosition, const SkColor4f &color, float position);
  SkColor4f at(float position, bool after) const;

  std::vector<SkColor4f> colors_;
  std::vector<float> positions_;
};

} // namespace rnwebos
