#include "SvgRootNode.h"

namespace rnwebos::svg {

RenderContext RootNode::rootContext(SkSize size, const Definitions &definitions) const {
  RenderContext context{
      .paint = paint,
      .currentColor = color.value_or(SkColor4f{0, 0, 0, 1}),
      .viewport = viewBox.present() ? SkSize::Make(viewBox.width, viewBox.height) : size,
      .definitions = &definitions,
  };
  inherit(context);
  return context;
}

void RootNode::renderInto(SkCanvas &canvas, SkSize size, float scale, float alpha) const {
  const Definitions definitions(*this);
  auto context = rootContext(size, definitions);
  context.alpha = alpha;
  const SkAutoCanvasRestore restore(&canvas, true);
  canvas.scale(scale, scale);
  canvas.concat(viewBoxTransform(viewBox, size.width(), size.height()));
  draw(canvas, context);
}

int RootNode::paintLayers(SkSize size) const {
  const Definitions definitions(*this);
  return contentPaintCount(rootContext(size, definitions));
}

void RootNode::changed() {
  if (onChange) onChange();
}

} // namespace rnwebos::svg
