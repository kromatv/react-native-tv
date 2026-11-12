#include "SvgUseNode.h"

namespace rnwebos::svg {

namespace {

RenderContext oneReferenceDeeper(const RenderContext &context) {
  RenderContext deeper = context;
  ++deeper.depth;
  return deeper;
}

} // namespace

bool SymbolNode::definesOnly() const {
  return true;
}

const Node *UseNode::target(const RenderContext &context) const {
  return context.canFollowReference() ? context.definitions->find(href) : nullptr;
}

SkMatrix UseNode::placement(const Node &target, const RenderContext &context) const {
  const auto &viewport = context.viewport;
  auto placed = SkMatrix::Translate(x.resolve(viewport.width()), y.resolve(viewport.height()));
  const auto *symbol = dynamic_cast<const SymbolNode *>(&target);
  if (symbol == nullptr) return placed;
  // react-native-svg sends 0 for a use without a size, which SVG reads as the whole viewport.
  const float boxWidth = width.resolve(viewport.width());
  const float boxHeight = height.resolve(viewport.height());
  placed.preConcat(viewBoxTransform(
      symbol->viewBox, boxWidth > 0 ? boxWidth : viewport.width(), boxHeight > 0 ? boxHeight : viewport.height()));
  return placed;
}

void UseNode::draw(SkCanvas &canvas, const RenderContext &context) const {
  const auto *node = target(context);
  if (node == nullptr) return;
  const SkAutoCanvasRestore restore(&canvas, true);
  canvas.concat(placement(*node, context));
  node->render(canvas, oneReferenceDeeper(context));
}

int UseNode::contentPaintCount(const RenderContext &context) const {
  const auto *node = target(context);
  return node == nullptr ? 0 : node->paintCount(oneReferenceDeeper(context));
}

SkRect UseNode::contentBounds(const RenderContext &context) const {
  const auto *node = target(context);
  if (node == nullptr) return SkRect::MakeEmpty();
  auto toUse = placement(*node, context);
  toUse.preConcat(node->matrix);
  return toUse.mapRect(node->bounds(oneReferenceDeeper(context)));
}

void UseNode::appendContentOutlines(
    std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const {
  const auto *node = target(context);
  if (node == nullptr) return;
  node->appendOutlines(outlines, SkMatrix::Concat(transform, placement(*node, context)), oneReferenceDeeper(context));
}

} // namespace rnwebos::svg
