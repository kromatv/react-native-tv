#pragma once

#include "SvgPaint.h"
#include "SvgPath.h"

#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace rntizen::svg {

/// One `<Stop>`: its offset from 0 to 1 and its color, `stopOpacity` included.
struct GradientStop {
  double offset{0};
  Rgba color;
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
  Affine transform;
};

/// The gradients an `<Svg>` defines, by `id`.
using GradientDefinitions = std::map<std::string, GradientDefinition, std::less<>>;

/// A gradient ready for Evas VG, in the `<Svg>` view's pixels, its stops sorted
/// and clamped to 0..1.
struct GradientPaint {
  bool radial{false};
  Point start;
  Point end;
  Point center;
  Point focal;
  double radius{0};
  std::vector<GradientStop> stops;
};

/// No paint, one color, or a gradient.
using ResolvedPaint = std::variant<std::monostate, Rgba, GradientPaint>;

/// react-native-svg's `gradient` prop: offset and ARGB color pairs. A color
/// passes through a 32-bit float on the way, so a stop whose alpha is neither
/// 0 nor 255 can lose the low bits of its blue (efl-rendering.md, section 8).
std::vector<GradientStop> gradientStops(const std::vector<float> &gradient);

/// What `definition` paints on a shape whose path, in user units, spans
/// `bounds`, drawn through `toPixels`, every stop's alpha multiplied by
/// `opacity`. SVG's rules apply: no stop paints nothing, one stop or a zero
/// length paints the last stop's color, and a bounding-box gradient on a shape
/// without width or height paints nothing. A radial gradient stretched unevenly
/// draws as a circle of the mean radius.
ResolvedPaint resolveGradient(
    const GradientDefinition &definition,
    const Bounds &bounds,
    const Affine &toPixels,
    double viewportWidth,
    double viewportHeight,
    double opacity);

} // namespace rntizen::svg
