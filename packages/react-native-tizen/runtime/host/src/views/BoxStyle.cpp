#include "BoxStyle.h"

#include "EvasColor.h"
#include "PixelGeometry.h"

#include <algorithm>

namespace rntizen {

namespace {

using facebook::react::Float;

CornerRadius cornerRadius(const facebook::react::CornerRadii &radii, Float scale) {
  return {.x = std::max(0, toPixels(radii.horizontal, scale)), .y = std::max(0, toPixels(radii.vertical, scale))};
}

int borderPixels(Float width, Float scale) {
  if (width <= 0) return 0;
  return std::max(1, toPixels(width, scale));
}

OutlineStyle outlineStyle(const facebook::react::ViewProps &props, Float scale) {
  if (props.outlineWidth <= 0 || !isVisible(props.outlineColor)) return {};
  return {
      .width = borderPixels(props.outlineWidth, scale),
      .offset = toPixels(props.outlineOffset, scale),
      .color = props.outlineColor,
  };
}

StrokePattern strokePattern(const facebook::react::BorderStyles &styles) {
  const auto style = styles.top;
  if (styles.left != style || styles.right != style || styles.bottom != style) return StrokePattern::Solid;
  switch (style) {
    case facebook::react::BorderStyle::Dashed:
      return StrokePattern::Dashed;
    case facebook::react::BorderStyle::Dotted:
      return StrokePattern::Dotted;
    case facebook::react::BorderStyle::Solid:
      return StrokePattern::Solid;
  }
  return StrokePattern::Solid;
}

std::vector<ShadowStyle> shadowStyles(const facebook::react::ViewProps &props, Float scale) {
  std::vector<ShadowStyle> shadows;
  if (props.shadowOpacity > 0 && isVisible(props.shadowColor)) {
    shadows.push_back({
        .offsetX = toPixels(props.shadowOffset.width, scale),
        .offsetY = toPixels(props.shadowOffset.height, scale),
        .blur = toPixels(props.shadowRadius * 2, scale),
        .spread = 0,
        .color = props.shadowColor,
        .opacity = static_cast<float>(props.shadowOpacity),
    });
  }
  for (const auto &shadow : props.boxShadow) {
    if (shadow.inset || !isVisible(shadow.color)) continue;
    shadows.push_back({
        .offsetX = toPixels(shadow.offsetX, scale),
        .offsetY = toPixels(shadow.offsetY, scale),
        .blur = std::max(0, toPixels(shadow.blurRadius, scale)),
        .spread = toPixels(shadow.spreadDistance, scale),
        .color = shadow.color,
        .opacity = 1,
    });
  }
  return shadows;
}

} // namespace

BoxStyle resolveBoxStyle(
    const facebook::react::ViewProps &props,
    const facebook::react::LayoutMetrics &metrics,
    bool forceClip) {
  const auto scale = metrics.pointScaleFactor;
  const auto border = props.resolveBorderMetrics(metrics);
  const auto &radii = border.borderRadii;
  const auto &widths = metrics.borderWidth;
  return {
      .backgroundColor = props.backgroundColor,
      .corners =
          {.topLeft = cornerRadius(radii.topLeft, scale),
           .topRight = cornerRadius(radii.topRight, scale),
           .bottomRight = cornerRadius(radii.bottomRight, scale),
           .bottomLeft = cornerRadius(radii.bottomLeft, scale)},
      .borderWidths =
          {.left = borderPixels(widths.left, scale),
           .top = borderPixels(widths.top, scale),
           .right = borderPixels(widths.right, scale),
           .bottom = borderPixels(widths.bottom, scale)},
      .borderColors = border.borderColors,
      .borderPattern = strokePattern(border.borderStyles),
      .outline = outlineStyle(props, scale),
      .shadows = shadowStyles(props, scale),
      .backgroundImages = props.backgroundImage,
      .pointScale = scale,
      .clips = forceClip || props.getClipsContentToBounds(),
  };
}

} // namespace rntizen
