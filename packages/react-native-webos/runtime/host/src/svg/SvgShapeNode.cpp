#include "SvgShapeNode.h"

#include "PathData.h"
#include "SvgBrushPaint.h"
#include "SvgPath.h"

namespace rnwebos::svg {

namespace {

std::optional<float> resolved(const std::optional<Length> &length, float reference) {
  if (!length) return std::nullopt;
  return length->resolve(reference);
}

} // namespace

void ShapeNode::forgetPath() {
  path_.reset();
  bounds_.reset();
}

const SkPath &ShapeNode::path(const RenderContext &context) const {
  if (!path_ || builtFor_ != context.viewport) {
    path_ = buildPath(context.viewport);
    bounds_.reset();
    builtFor_ = context.viewport;
  }
  if (const auto fillType = fillTypeOf(context.paint.fillRule); path_->getFillType() != fillType) {
    path_->setFillType(fillType);
  }
  return *path_;
}

const SkRect &ShapeNode::fillBounds(const RenderContext &context) const {
  const auto &shape = path(context);
  if (!bounds_) bounds_ = shape.computeTightBounds();
  return *bounds_;
}

void ShapeNode::draw(SkCanvas &canvas, const RenderContext &context) const {
  const auto &shape = path(context);
  if (shape.isEmpty()) return;
  const auto bounds = paintsWithReference(context.paint) ? fillBounds(context) : SkRect::MakeEmpty();
  if (const auto fill = fillPaint(context, bounds)) canvas.drawPath(shape, *fill);
  if (const auto stroke = strokePaint(context, bounds)) canvas.drawPath(shape, *stroke);
}

int ShapeNode::contentPaintCount(const RenderContext &context) const {
  using enum Brush::Kind;
  const auto &paint = context.paint;
  const bool strokes = paint.stroke.kind != None && paint.strokeWidth.value > 0;
  return (paint.fill.kind != None ? 1 : 0) + (strokes ? 1 : 0);
}

SkRect ShapeNode::contentBounds(const RenderContext &context) const {
  return fillBounds(context);
}

void ShapeNode::appendContentOutlines(
    std::vector<SkPath> &outlines, const SkMatrix &transform, const RenderContext &context) const {
  const auto &shape = path(context);
  if (!shape.isEmpty()) outlines.push_back(shape.makeTransform(transform));
}

SkPath PathGeometry::path(SkSize) const {
  return parsePathData(data);
}

SkPath CircleGeometry::path(SkSize viewport) const {
  const float radius = r.resolve(diagonalOf(viewport));
  return ellipsePath({cx.resolve(viewport.width()), cy.resolve(viewport.height())}, radius, radius);
}

SkPath EllipseGeometry::path(SkSize viewport) const {
  return ellipsePath(
      {cx.resolve(viewport.width()), cy.resolve(viewport.height())},
      rx.resolve(viewport.width()),
      ry.resolve(viewport.height()));
}

SkPath RectGeometry::path(SkSize viewport) const {
  return rectPath(
      {x.resolve(viewport.width()), y.resolve(viewport.height())},
      width.resolve(viewport.width()),
      height.resolve(viewport.height()),
      resolved(rx, viewport.width()),
      resolved(ry, viewport.height()));
}

SkPath LineGeometry::path(SkSize viewport) const {
  return linePath(
      {x1.resolve(viewport.width()), y1.resolve(viewport.height())},
      {x2.resolve(viewport.width()), y2.resolve(viewport.height())});
}

} // namespace rnwebos::svg
