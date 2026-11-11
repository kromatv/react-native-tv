#pragma once

#include "SvgGroupNode.h"

namespace rnwebos::svg {

/// `<ClipPath>`: what its children fill is the area a node naming it keeps.
class ClipPathNode final : public GroupNode {
 public:
  bool definesOnly() const override;

  /// The kept area in the user space of the node that names the clip, by
  /// react-native-svg's rule (`VirtualView.getClipPath` on Android): the
  /// node's `clipRule` picks even-odd over all the children's outlines
  /// together (0, its default) or the union of them (1).
  SkPath outline(int clipRule, const RenderContext &context) const;
};

} // namespace rnwebos::svg
