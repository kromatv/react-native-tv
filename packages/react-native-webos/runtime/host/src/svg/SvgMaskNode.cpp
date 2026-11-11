#include "SvgMaskNode.h"

#include "include/core/SkBlendMode.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkPaint.h"
#include "include/effects/SkLumaColorFilter.h"

namespace rnwebos::svg {

bool MaskNode::definesOnly() const {
  return true;
}

SkRect MaskNode::area(const SkRect &target, SkSize viewport) const {
  if (areaInUserSpace) {
    return SkRect::MakeXYWH(
        x.resolve(viewport.width()),
        y.resolve(viewport.height()),
        width.resolve(viewport.width()),
        height.resolve(viewport.height()));
  }
  return SkRect::MakeXYWH(
      target.left() + x.fractionOf(target.width()),
      target.top() + y.fractionOf(target.height()),
      width.fractionOf(target.width()),
      height.fractionOf(target.height()));
}

void MaskNode::cut(SkCanvas &canvas, const SkRect &target, const RenderContext &context) const {
  SkPaint keep;
  keep.setBlendMode(SkBlendMode::kDstIn);
  if (!alphaOnly) keep.setColorFilter(SkLumaColorFilter::Make());
  // No bounds: where the mask draws nothing, the layer must still erase the content.
  canvas.saveLayer(nullptr, &keep);
  canvas.clipRect(area(target, context.viewport), true);
  if (contentInBoundingBox) {
    canvas.translate(target.left(), target.top());
    canvas.scale(target.width(), target.height());
  }
  RenderContext inside{
      .paint = paint,
      .currentColor = color.value_or(context.currentColor),
      .viewport = context.viewport,
      .definitions = context.definitions,
      .depth = context.depth + 1,
  };
  inherit(inside);
  GroupNode::draw(canvas, inside);
  canvas.restore();
}

} // namespace rnwebos::svg
