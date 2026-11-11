#include "SvgGroupNode.h"

#include <algorithm>

namespace rnwebos::svg {

void GroupNode::draw(SkCanvas &canvas, const RenderContext &context) const {
  for (const auto *child : children()) {
    if (!child->definesOnly()) child->render(canvas, context);
  }
}

int GroupNode::contentPaintCount(const RenderContext &context) const {
  int count = 0;
  for (const auto *child : children()) {
    if (!child->definesOnly()) count += child->paintCount(context);
    if (count > 1) return 2;
  }
  return count;
}

SkRect GroupNode::contentBounds(const RenderContext &context) const {
  bool any = false;
  SkRect bounds = SkRect::MakeEmpty();
  for (const auto *child : children()) {
    if (child->definesOnly() || child->hidden) continue;
    const auto box = child->matrix.mapRect(child->bounds(context));
    bounds = any
        ? SkRect::MakeLTRB(
              std::min(bounds.left(), box.left()),
              std::min(bounds.top(), box.top()),
              std::max(bounds.right(), box.right()),
              std::max(bounds.bottom(), box.bottom()))
        : box;
    any = true;
  }
  return bounds;
}

void GroupNode::appendContentOutlines(
    std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const {
  for (const auto *child : children()) {
    if (!child->definesOnly()) child->appendOutlines(outlines, transform, context);
  }
}

void GroupNode::inherit(RenderContext &context) const {
  context.font = font.over(context.font);
}

bool DefinitionsNode::definesOnly() const {
  return true;
}

} // namespace rnwebos::svg
