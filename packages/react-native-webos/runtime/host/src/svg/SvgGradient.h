#pragma once

#include "SvgPaint.h"

#include "include/core/SkMatrix.h"
#include "include/core/SkRect.h"
#include "include/core/SkRefCnt.h"
#include "include/core/SkShader.h"
#include "include/core/SkSize.h"

#include <variant>
#include <vector>

namespace rnwebos::svg {

/// One `<Stop>`: its offset from 0 to 1 and its color, `stopOpacity` included.
struct GradientStop {
  float offset{0};
  SkColor4f color{0, 0, 0, 1};
};

/// A `<LinearGradient>` or `<RadialGradient>` as react-native-svg sends it.
struct GradientDefinition {
  bool radial{false};
  Length x1;
  Length y1;
  Length x2;
  Length y2;
  Length cx;
  Length cy;
  Length rx;
  Length ry;
  Length fx;
  Length fy;
  std::vector<GradientStop> stops;
  bool userSpace{false};
  SkMatrix transform;
};

/// No paint, one color, or a shader in the painted shape's user space.
using GradientPaint = std::variant<std::monostate, SkColor4f, sk_sp<SkShader>>;

/// react-native-svg's `gradient` prop: offset and ARGB color pairs. A color
/// passes through a 32-bit float on the way, so a stop whose alpha is neither
/// 0 nor 255 can lose the low bits of its blue (`extractGradient.ts` packs
/// it, codegen's `Float` carries it).
std::vector<GradientStop> gradientStops(const std::vector<float> &gradient);

/// What `definition` paints on a shape whose geometry, in user units, spans
/// `bounds`, in an `<Svg>` whose percentages resolve against `viewport`. SVG's
/// rules apply: no stop paints nothing, one stop or a zero length paints the
/// last stop's color, and a bounding-box gradient on a shape without width or
/// height paints nothing.
GradientPaint gradientPaint(const GradientDefinition &definition, const SkRect &bounds, SkSize viewport);

} // namespace rnwebos::svg
