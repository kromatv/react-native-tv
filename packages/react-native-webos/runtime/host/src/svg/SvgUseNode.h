#pragma once

#include "SvgGroupNode.h"
#include "SvgViewBox.h"

#include <string>

namespace rnwebos::svg {

/// `<Symbol>`: drawn only by a `<Use>`, which fits its viewBox into the use's
/// width and height.
class SymbolNode final : public GroupNode {
 public:
  SvgViewBox viewBox;

  bool definesOnly() const override;
};

/// `<Use>`: draws the node named `href` at `x`, `y`, inheriting from the use
/// in place of the target's own ancestors.
class UseNode final : public Node {
 public:
  std::string href;
  Length x;
  Length y;
  Length width;
  Length height;

 private:
  void draw(SkCanvas &canvas, const RenderContext &context) const override;
  int contentPaintCount(const RenderContext &context) const override;
  SkRect contentBounds(const RenderContext &context) const override;
  void appendContentOutlines(
      std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const override;

  const Node *target(const RenderContext &context) const;
  /// From the target's parent space to the use's content space.
  SkMatrix placement(const Node &target, const RenderContext &context) const;
};

} // namespace rnwebos::svg
