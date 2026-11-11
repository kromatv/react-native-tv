#pragma once

#include <folly/dynamic.h>

#include <optional>
#include <string>

namespace rnwebos::svg {

enum class TextAnchor { Start, Middle, End };

/// The font a text draws with, as it reaches the text through its ancestors.
struct Font {
  std::string family;
  float size{12};
  int weight{400};
  bool italic{false};
  TextAnchor anchor{TextAnchor::Start};

  bool operator==(const Font &) const = default;
};

/// The font properties one node sets, from react-native-svg's `font` prop
/// (`extractFont`); the others come from its ancestors.
struct FontAttributes {
  std::optional<std::string> family;
  /// User units, or a multiple of the inherited size when `relativeSize`.
  std::optional<float> size;
  bool relativeSize{false};
  std::optional<int> weight;
  std::optional<bool> italic;
  std::optional<TextAnchor> anchor;

  /// `inherited` with the node's own properties over it.
  Font over(const Font &inherited) const;
};

FontAttributes fontAttributesFromProp(const folly::dynamic &font);

} // namespace rnwebos::svg
