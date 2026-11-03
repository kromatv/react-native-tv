#pragma once

#include <folly/dynamic.h>
#include <react/renderer/graphics/Color.h>

#include <bitset>
#include <optional>
#include <string>
#include <vector>

namespace rntizen::svg {

/// A color with straight (not premultiplied) components from 0 to 1.
struct Rgba {
  double red{0};
  double green{0};
  double blue{0};
  double alpha{1};
};

struct Brush {
  enum class Kind { None, Color, CurrentColor, Reference };

  Kind kind{Kind::None};
  Rgba color;
  std::string reference;
};

/// A react-native-svg length: user units, or a percentage of a reference.
struct Length {
  double value{0};
  bool percent{false};

  double resolve(double reference) const;
};

enum class PaintAttribute : std::size_t {
  Fill,
  FillOpacity,
  FillRule,
  Stroke,
  StrokeOpacity,
  StrokeWidth,
  StrokeLinecap,
  StrokeLinejoin,
  StrokeMiterlimit,
  Count,
};

using PaintAttributes = std::bitset<static_cast<std::size_t>(PaintAttribute::Count)>;

/// The presentation attributes a node draws with. `listed` holds the ones the
/// node sets itself or inherits, which its own children inherit in turn.
struct Paint {
  Brush fill{.kind = Brush::Kind::Color};
  double fillOpacity{1};
  int fillRule{1};
  Brush stroke;
  double strokeOpacity{1};
  Length strokeWidth{.value = 1};
  int strokeLinecap{0};
  int strokeLinejoin{0};
  double strokeMiterlimit{4};
  PaintAttributes listed;
};

/// react-native-svg's inheritance: an attribute the node does not list comes
/// from its parent when the parent lists it, else keeps the node's default.
Paint inheritPaint(const Paint &own, const Paint &parent);

PaintAttributes paintAttributesFromPropList(const std::vector<std::string> &propList);

/// A brush as react-native-svg's `extractBrush` sends it: `{type, payload}`, or
/// `{type, brushRef}` for `url(#id)`.
Brush brushFromProp(const folly::dynamic &value);

/// A length prop (`NumberProp`): a number, or a string such as `"2"`, `"2px"`, `"50%"`.
std::optional<Length> lengthFromProp(const folly::dynamic &value);

std::optional<Rgba> rgbaFromColor(const facebook::react::SharedColor &color);

} // namespace rntizen::svg
