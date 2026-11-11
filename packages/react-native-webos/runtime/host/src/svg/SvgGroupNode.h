#pragma once

#include "SvgFont.h"
#include "SvgNode.h"

namespace rnwebos::svg {

/// `<G>`: draws its children in order, each inheriting its paint and font.
/// Children that only define something are passed over.
class GroupNode : public Node {
 public:
  FontAttributes font;

 protected:
  void draw(SkCanvas &canvas, const RenderContext &context) const override;
  int contentPaintCount(const RenderContext &context) const override;
  SkRect contentBounds(const RenderContext &context) const override;
  void appendContentOutlines(
      std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const override;
  void inherit(RenderContext &context) const override;
};

/// `<Defs>`, and any element whose children are drawn only by reference.
class DefinitionsNode final : public GroupNode {
 public:
  bool definesOnly() const override;
};

} // namespace rnwebos::svg
