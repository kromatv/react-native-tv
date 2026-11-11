#pragma once

#include "SvgGroupNode.h"
#include "SvgViewBox.h"

#include "include/core/SkShader.h"

namespace rnwebos::svg {

/// `<Pattern>`: its children drawn into one tile, which repeats over whatever
/// paints with `url(#id)`.
class PatternNode final : public GroupNode {
 public:
  Length x{.value = 0, .percent = true};
  Length y{.value = 0, .percent = true};
  Length width{.value = 100, .percent = true};
  Length height{.value = 100, .percent = true};
  /// `patternUnits="userSpaceOnUse"`: the tile is in user units, not a share
  /// of the painted geometry's bounding box.
  bool tileInUserSpace{false};
  /// `patternContentUnits="objectBoundingBox"`: the children's unit square is
  /// the painted geometry's bounding box. A viewBox takes precedence.
  bool contentInBoundingBox{false};
  SkMatrix transform;
  SvgViewBox viewBox;

  bool definesOnly() const override;

  /// The tile repeated from its origin, for a geometry spanning `bounds` in
  /// its user space. Null when the tile has no size.
  sk_sp<SkShader> shader(const SkRect &bounds, const RenderContext &context) const;

 private:
  SkRect tile(const SkRect &bounds, SkSize viewport) const;
};

} // namespace rnwebos::svg
