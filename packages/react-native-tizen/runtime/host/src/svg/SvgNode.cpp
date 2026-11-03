#include "SvgNode.h"

#include <cmath>

namespace rntizen::svg {

namespace {

ResolvedPaint resolveBrush(const Brush &brush, double opacity, const Path &path, const DrawContext &context) {
  using enum Brush::Kind;
  Rgba color;
  switch (brush.kind) {
    case None:
      return {};
    case Color:
      color = brush.color;
      break;
    case CurrentColor:
      color = context.currentColor;
      break;
    case Reference: {
      if (context.gradients == nullptr) return {};
      const auto found = context.gradients->find(brush.reference);
      if (found == context.gradients->end()) return {};
      return resolveGradient(
          found->second,
          path.bounds(),
          context.transform,
          context.viewportWidth,
          context.viewportHeight,
          opacity * context.opacity);
    }
  }
  color.alpha *= opacity * context.opacity;
  return color;
}

void assign(const ResolvedPaint &paint, std::optional<Rgba> &color, std::optional<GradientPaint> &gradient) {
  if (const auto *solid = std::get_if<Rgba>(&paint); solid != nullptr && solid->alpha > 0) color = *solid;
  if (const auto *ramp = std::get_if<GradientPaint>(&paint)) gradient = *ramp;
}

} // namespace

double DrawContext::viewportDiagonal() const {
  return std::sqrt((viewportWidth * viewportWidth + viewportHeight * viewportHeight) / 2);
}

DrawnShape drawnShape(const Path &path, const DrawContext &context) {
  const auto &paint = context.paint;
  DrawnShape shape{
      .path = path.transformed(context.transform),
      .strokeWidth = paint.strokeWidth.resolve(context.viewportDiagonal()) * context.transform.lengthScale(),
      .strokeLinecap = paint.strokeLinecap,
      .strokeLinejoin = paint.strokeLinejoin,
  };
  assign(resolveBrush(paint.fill, paint.fillOpacity, path, context), shape.fill, shape.fillGradient);
  if (shape.strokeWidth > 0) {
    assign(resolveBrush(paint.stroke, paint.strokeOpacity, path, context), shape.stroke, shape.strokeGradient);
  }
  return shape;
}

} // namespace rntizen::svg
