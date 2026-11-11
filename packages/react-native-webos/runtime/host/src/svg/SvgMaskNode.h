#pragma once

#include "SvgGroupNode.h"

namespace rnwebos::svg {

/// `<Mask>`: its children, drawn as luminance or alpha, are how much of a
/// node naming it shows.
class MaskNode final : public GroupNode {
 public:
  Length x{.value = 0, .percent = true};
  Length y{.value = 0, .percent = true};
  Length width{.value = 100, .percent = true};
  Length height{.value = 100, .percent = true};
  /// `maskUnits="userSpaceOnUse"`: the area is in user units, not a share of
  /// the masked node's bounding box.
  bool areaInUserSpace{false};
  /// `maskContentUnits="objectBoundingBox"`: the children's unit square is the
  /// masked node's bounding box.
  bool contentInBoundingBox{false};
  /// `mask-type: alpha`: the children's alpha masks, not their luminance.
  bool alphaOnly{false};

  bool definesOnly() const override;

  /// Keeps of the canvas's top layer what the mask lets through, for a node
  /// whose content spans `target`, in that node's user space.
  void cut(SkCanvas &canvas, const SkRect &target, const RenderContext &context) const;

 private:
  SkRect area(const SkRect &target, SkSize viewport) const;
};

} // namespace rnwebos::svg
