#include "BoxPainter.h"

#include "SkiaColor.h"

#include "include/core/SkBlurTypes.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/effects/SkDashPathEffect.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace rnwebos {

namespace {

using facebook::react::SharedColor;

SkVector radius(const CornerRadius &corner) {
  return {static_cast<float>(corner.x), static_cast<float>(corner.y)};
}

SkRRect roundedRectIn(const SkRect &rect, const Corners &corners) {
  if (corners.isSquare()) return SkRRect::MakeRect(rect);
  const std::array<SkVector, 4> radii{
      radius(corners.topLeft), radius(corners.topRight), radius(corners.bottomRight), radius(corners.bottomLeft)};
  SkRRect shape;
  shape.setRectRadii(rect, radii.data());
  return shape;
}

SkPaint fill(const SkColor4f &color) {
  SkPaint paint;
  paint.setAntiAlias(true);
  paint.setColor4f(color);
  return paint;
}

bool hasBorder(const BoxStyle &style) {
  const auto &c = style.borderColors;
  return !style.borderWidths.isZero() &&
      (isVisible(c.left) || isVisible(c.top) || isVisible(c.right) || isVisible(c.bottom));
}

bool hasOneColor(const BoxStyle &style) {
  const auto &c = style.borderColors;
  return c.left == c.top && c.top == c.right && c.right == c.bottom;
}

// A blur of radius b is a Gaussian of deviation b / 2, which fades out over
// three deviations.
int shadowReach(const ShadowStyle &shadow) {
  return shadow.spread + (shadow.blur * 3 + 1) / 2;
}

// Chromium's dashes: three widths long and two apart below 3 px, two and one
// from 3 px; dots are one width across, a width apart.
std::array<float, 2> dashIntervals(StrokePattern pattern, float width, float length) {
  const bool dotted = pattern == StrokePattern::Dotted;
  const bool thin = width < 3;
  const float dashWidths = thin ? 3 : 2;
  const float gapWidths = thin ? 2 : 1;
  const float dash = dotted ? 0 : width * dashWidths;
  const float gap = dotted ? width * 2 : width * gapWidths;
  const float period = dash + gap;
  const auto count = std::max(1.0f, std::round(length / period));
  return {dash, length / count - dash};
}

} // namespace

SkRRect roundedRect(PixelSize size, const Corners &corners) {
  return roundedRectIn(SkRect::MakeWH(static_cast<float>(size.width), static_cast<float>(size.height)), corners);
}

SkRRect insetRoundedRect(PixelSize size, const Corners &corners, const Edges &widths) {
  const auto rect = SkRect::MakeLTRB(
      static_cast<float>(widths.left),
      static_cast<float>(widths.top),
      static_cast<float>(std::max(widths.left, size.width - widths.right)),
      static_cast<float>(std::max(widths.top, size.height - widths.bottom)));
  return roundedRectIn(rect, corners.inset(widths));
}

bool BoxPainter::update(const BoxStyle &style) {
  if (style == style_) return false;
  style_ = style;
  return true;
}

const BoxStyle &BoxPainter::style() const {
  return style_;
}

int BoxPainter::layerCount() const {
  return static_cast<int>(style_.shadows.size() + style_.backgroundImages.size()) +
      (isVisible(style_.backgroundColor) ? 1 : 0) + (hasBorder(style_) ? 1 : 0) + (style_.outline.width > 0 ? 1 : 0);
}

Edges BoxPainter::overflow() const {
  Edges edges;
  for (const auto &shadow : style_.shadows) {
    const auto grow = shadowReach(shadow);
    edges.left = std::max(edges.left, grow - shadow.offsetX);
    edges.right = std::max(edges.right, grow + shadow.offsetX);
    edges.top = std::max(edges.top, grow - shadow.offsetY);
    edges.bottom = std::max(edges.bottom, grow + shadow.offsetY);
  }
  if (style_.outline.width > 0) {
    const auto grow = style_.outline.offset + style_.outline.width;
    edges = {std::max(edges.left, grow), std::max(edges.top, grow), std::max(edges.right, grow), std::max(edges.bottom, grow)};
  }
  return edges;
}

void BoxPainter::drawBelowContent(SkCanvas &canvas, PixelSize size, float alpha) const {
  if (size.width <= 0 || size.height <= 0) return;
  const auto box = roundedRect(size, style_.corners);
  drawShadows(canvas, box, alpha);
  if (isVisible(style_.backgroundColor)) {
    const auto paint = fill(skColor(style_.backgroundColor, alpha));
    if (box.isRect()) {
      canvas.drawRect(box.rect(), paint);
    } else {
      canvas.drawRRect(box, paint);
    }
  }
  if (!style_.backgroundImages.empty()) {
    const auto area = insetRoundedRect(size, style_.corners, style_.borderWidths);
    drawBackgroundImages(canvas, area, style_.backgroundImages, static_cast<float>(style_.pointScale), alpha);
  }
}

void BoxPainter::drawShadows(SkCanvas &canvas, const SkRRect &box, float alpha) const {
  if (style_.shadows.empty()) return;
  // CSS draws no shadow under the box itself, which shows through a
  // translucent background.
  const bool hidden = isOpaque(style_.backgroundColor);
  SkAutoCanvasRestore restore(&canvas, !hidden);
  if (!hidden) canvas.clipRRect(box, SkClipOp::kDifference, true);
  for (auto shadow = style_.shadows.rbegin(); shadow != style_.shadows.rend(); ++shadow) {
    auto shape = box;
    shape.outset(static_cast<float>(shadow->spread), static_cast<float>(shadow->spread));
    shape.offset(static_cast<float>(shadow->offsetX), static_cast<float>(shadow->offsetY));
    auto paint = fill(skColor(shadow->color, shadow->opacity * alpha));
    if (shadow->blur > 0) {
      paint.setMaskFilter(SkMaskFilter::MakeBlur(kNormal_SkBlurStyle, static_cast<float>(shadow->blur) / 2));
    }
    canvas.drawRRect(shape, paint);
  }
}

void BoxPainter::drawAboveChildren(SkCanvas &canvas, PixelSize size, float alpha) const {
  if (size.width <= 0 || size.height <= 0) return;
  if (hasBorder(style_)) drawBorder(canvas, roundedRect(size, style_.corners), size, alpha);
  if (style_.outline.width > 0) drawOutline(canvas, size, alpha);
}

void BoxPainter::drawBorder(SkCanvas &canvas, const SkRRect &box, PixelSize size, float alpha) const {
  if (style_.borderPattern != StrokePattern::Solid && style_.borderWidths.isUniform() && hasOneColor(style_)) {
    drawPatternedBorder(canvas, box, alpha);
    return;
  }
  const auto inner = insetRoundedRect(size, style_.corners, style_.borderWidths);
  const auto &colors = style_.borderColors;
  if (hasOneColor(style_)) {
    canvas.drawDRRect(box, inner, fill(skColor(colors.top, alpha)));
    return;
  }
  // Each side is the ring cut along the diagonals of its two corners.
  const auto w = static_cast<float>(size.width);
  const auto h = static_cast<float>(size.height);
  const auto &i = inner.rect();
  const std::array<SkPoint, 4> outerCorners{{{0, 0}, {w, 0}, {w, h}, {0, h}}};
  const std::array<SkPoint, 4> innerCorners{
      {{i.left(), i.top()}, {i.right(), i.top()}, {i.right(), i.bottom()}, {i.left(), i.bottom()}}};
  const std::array<const SharedColor *, 4> sides{&colors.top, &colors.right, &colors.bottom, &colors.left};
  for (std::size_t side = 0; side < 4; ++side) {
    if (!isVisible(*sides[side])) continue;
    const auto next = (side + 1) % 4;
    const std::array<SkPoint, 4> quad{outerCorners[side], outerCorners[next], innerCorners[next], innerCorners[side]};
    SkAutoCanvasRestore restore(&canvas, true);
    canvas.clipPath(SkPath::Polygon(quad, true), true);
    canvas.drawDRRect(box, inner, fill(skColor(*sides[side], alpha)));
  }
}

void BoxPainter::drawPatternedBorder(SkCanvas &canvas, const SkRRect &box, float alpha) const {
  const auto width = static_cast<float>(style_.borderWidths.top);
  auto centerline = box;
  centerline.inset(width / 2, width / 2);
  const auto &rect = centerline.rect();
  const auto length = 2 * (rect.width() + rect.height());
  if (length <= 0) return;
  const auto intervals = dashIntervals(style_.borderPattern, width, length);
  auto paint = fill(skColor(style_.borderColors.top, alpha));
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(width);
  if (style_.borderPattern == StrokePattern::Dotted) paint.setStrokeCap(SkPaint::kRound_Cap);
  paint.setPathEffect(SkDashPathEffect::Make(intervals, 0));
  canvas.drawRRect(centerline, paint);
}

void BoxPainter::drawOutline(SkCanvas &canvas, PixelSize size, float alpha) const {
  const auto &outline = style_.outline;
  const auto width = static_cast<float>(outline.width);
  const auto grow = static_cast<float>(outline.offset) + width / 2;
  const auto rect =
      SkRect::MakeWH(static_cast<float>(size.width), static_cast<float>(size.height)).makeOutset(grow, grow);
  if (rect.isEmpty()) return;
  const auto shape = roundedRectIn(rect, style_.corners.grown(outline.offset + outline.width / 2));
  auto paint = fill(skColor(outline.color, alpha));
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(width);
  canvas.drawRRect(shape, paint);
}

void BoxPainter::clipContent(SkCanvas &canvas, PixelSize size) const {
  const auto shape = insetRoundedRect(size, style_.corners, style_.borderWidths);
  if (shape.isRect()) {
    canvas.clipRect(shape.rect(), false);
  } else {
    canvas.clipRRect(shape, true);
  }
}

} // namespace rnwebos
