#pragma once

#include <folly/dynamic.h>
#include <react/renderer/graphics/Color.h>

#include "include/core/SkColor.h"

#include <bitset>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace rnwebos::svg {

struct Brush {
  enum class Kind { None, Color, CurrentColor, Reference };

  Kind kind{Kind::None};
  SkColor4f color{0, 0, 0, 1};
  std::string reference;
};

/// A react-native-svg length: user units, or a percentage of a reference.
struct Length {
  float value{0};
  bool percent{false};

  bool operator==(const Length &) const = default;

  float resolve(float reference) const;
  /// The length as a share of `reference`, for bounding-box units: `0.5` and
  /// `"50%"` are both half of it.
  float fractionOf(float reference) const;
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
  StrokeDasharray,
  StrokeDashoffset,
  Count,
};

using PaintAttributes = std::bitset<static_cast<std::size_t>(PaintAttribute::Count)>;

/// The presentation attributes a node draws with. `listed` holds the ones the
/// node sets itself or inherits, which its own children inherit in turn.
struct Paint {
  Brush fill{.kind = Brush::Kind::Color};
  float fillOpacity{1};
  int fillRule{1};
  Brush stroke;
  float strokeOpacity{1};
  Length strokeWidth{.value = 1};
  int strokeLinecap{0};
  int strokeLinejoin{0};
  float strokeMiterlimit{4};
  std::vector<Length> strokeDasharray;
  float strokeDashoffset{0};
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

/// A list of lengths, as `strokeDasharray` or a text's `x` comes: an array of
/// `NumberProp`, or one alone. Entries that are no length are left out.
std::vector<Length> lengthsFromProp(const folly::dynamic &value);

std::optional<SkColor4f> colorFromProp(const facebook::react::SharedColor &color);

/// A packed `0xAARRGGBB` color, as React Native processes one.
SkColor4f colorFromArgb(std::uint32_t argb);

} // namespace rnwebos::svg
