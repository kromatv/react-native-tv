#include "SvgBrushPaint.h"

#include "SvgGradientNode.h"

#include "include/effects/SkDashPathEffect.h"

#include <algorithm>
#include <variant>
#include <vector>

namespace rnwebos::svg {

namespace {

constexpr int kEvenOdd = 0;
constexpr int kRoundCap = 1;
constexpr int kSquareCap = 2;
constexpr int kRoundJoin = 1;
constexpr int kBevelJoin = 2;

std::optional<SkPaint> solid(SkColor4f color, float opacity) {
  color.fA *= opacity;
  if (color.fA <= 0) return std::nullopt;
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor4f(color);
  return paint;
}

std::optional<SkPaint> referenced(
    const Brush &brush, float opacity, const RenderContext &context, const SkRect &bounds) {
  if (context.definitions == nullptr || opacity <= 0) return std::nullopt;
  const auto *gradient = context.definitions->findAs<GradientNode>(brush.reference);
  if (gradient == nullptr) return std::nullopt;
  auto resolved = gradientPaint(gradient->definition, bounds, context.viewport);
  if (const auto *color = std::get_if<SkColor4f>(&resolved)) return solid(*color, opacity);
  auto *shader = std::get_if<sk_sp<SkShader>>(&resolved);
  if (shader == nullptr) return std::nullopt;
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setShader(std::move(*shader));
  paint.setAlphaf(std::min(opacity, 1.0F));
  return paint;
}

std::optional<SkPaint> paintOf(const Brush &brush, float opacity, const RenderContext &context, const SkRect &bounds) {
  using enum Brush::Kind;
  opacity = std::clamp(opacity, 0.0F, 1.0F) * context.alpha;
  switch (brush.kind) {
    case None:
      return std::nullopt;
    case Color:
      return solid(brush.color, opacity);
    case CurrentColor:
      return solid(context.currentColor, opacity);
    case Reference:
      return referenced(brush, opacity, context, bounds);
  }
  return std::nullopt;
}

SkPaint::Cap capOf(int linecap) {
  if (linecap == kRoundCap) return SkPaint::kRound_Cap;
  return linecap == kSquareCap ? SkPaint::kSquare_Cap : SkPaint::kButt_Cap;
}

SkPaint::Join joinOf(int linejoin) {
  if (linejoin == kRoundJoin) return SkPaint::kRound_Join;
  return linejoin == kBevelJoin ? SkPaint::kBevel_Join : SkPaint::kMiter_Join;
}

sk_sp<SkPathEffect> dashesOf(const Paint &paint, float reference) {
  if (paint.strokeDasharray.empty()) return nullptr;
  std::vector<float> intervals;
  intervals.reserve(paint.strokeDasharray.size() * 2);
  for (const auto &length : paint.strokeDasharray) intervals.push_back(length.resolve(reference));
  // SVG repeats an odd list once to make it even; Skia takes none but even ones.
  if (intervals.size() % 2 == 1) intervals.insert(intervals.end(), intervals.begin(), intervals.end());
  return SkDashPathEffect::Make(intervals, paint.strokeDashoffset);
}

} // namespace

bool paintsWithReference(const Paint &paint) {
  return paint.fill.kind == Brush::Kind::Reference || paint.stroke.kind == Brush::Kind::Reference;
}

std::optional<SkPaint> fillPaint(const RenderContext &context, const SkRect &bounds) {
  return paintOf(context.paint.fill, context.paint.fillOpacity, context, bounds);
}

std::optional<SkPaint> strokePaint(const RenderContext &context, const SkRect &bounds) {
  const auto &attributes = context.paint;
  const float width = attributes.strokeWidth.resolve(context.viewportDiagonal());
  if (width <= 0) return std::nullopt;
  auto paint = paintOf(attributes.stroke, attributes.strokeOpacity, context, bounds);
  if (!paint) return std::nullopt;
  paint->setStyle(SkPaint::kStroke_Style);
  paint->setStrokeWidth(width);
  paint->setStrokeCap(capOf(attributes.strokeLinecap));
  paint->setStrokeJoin(joinOf(attributes.strokeLinejoin));
  paint->setStrokeMiter(attributes.strokeMiterlimit);
  paint->setPathEffect(dashesOf(attributes, context.viewportDiagonal()));
  return paint;
}

SkPathFillType fillTypeOf(int fillRule) {
  return fillRule == kEvenOdd ? SkPathFillType::kEvenOdd : SkPathFillType::kWinding;
}

} // namespace rnwebos::svg
