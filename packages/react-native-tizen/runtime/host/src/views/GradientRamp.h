#pragma once

#include "Premultiplied.h"

#include <react/renderer/graphics/ColorStop.h>

#include <vector>

namespace rntizen {

/// The colors along a CSS gradient line: React Native's color stops fixed up as
/// CSS Images 4 orders them, transition hints included, and interpolated in
/// premultiplied RGBA as browsers do (efl-rendering.md, section 4).
class GradientRamp {
 public:
  /// `stops` as React Native parsed them, on a line `lineLength` pixels long,
  /// a length position being in points at `pointScale` pixels each.
  GradientRamp(const std::vector<facebook::react::ColorStop> &stops, float lineLength, float pointScale);

  bool empty() const;
  /// The color at `t`, the fraction of the line from its start.
  Premultiplied at(float t) const;
  /// `at(t)`, looking for `t`'s stops from `segment` first and leaving it
  /// there, so a walk along the line skips the search with the same colors.
  Premultiplied at(float t, std::size_t &segment) const;

 private:
  struct Stop {
    float position;
    Premultiplied color;
    float hint;
  };

  Premultiplied between(const Stop &from, const Stop &to, float t) const;

  std::vector<Stop> stops_;
};

} // namespace rntizen
