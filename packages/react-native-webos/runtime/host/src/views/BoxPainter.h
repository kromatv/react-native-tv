#pragma once

#include "BoxStyle.h"
#include "PixelGeometry.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkRRect.h"

namespace rnwebos {

/// Draws a view's own box with Skia: shadows, background color and gradients
/// under the content, border and outline over the children. Coordinates are
/// the view's own pixels, its border box at the origin.
class BoxPainter final {
 public:
  /// Returns true when the style changed.
  bool update(const BoxStyle &style);
  const BoxStyle &style() const;

  /// Shadows, the background color, then the gradients, each at `alpha`.
  void drawBelowContent(SkCanvas &canvas, PixelSize size, float alpha) const;
  /// The border, then the outline.
  void drawAboveChildren(SkCanvas &canvas, PixelSize size, float alpha) const;
  /// Clips to the padding box, rounded corners included.
  void clipContent(SkCanvas &canvas, PixelSize size) const;

  /// How far shadows and the outline draw past the box on each side.
  Edges overflow() const;
  /// How many things the box paints: with one, an opacity can go into its paint.
  int layerCount() const;

 private:
  void drawShadows(SkCanvas &canvas, const SkRRect &box, float alpha) const;
  void drawBorder(SkCanvas &canvas, const SkRRect &box, PixelSize size, float alpha) const;
  void drawPatternedBorder(SkCanvas &canvas, const SkRRect &box, float alpha) const;
  void drawOutline(SkCanvas &canvas, PixelSize size, float alpha) const;

  BoxStyle style_;
};

/// The rounded rectangle of a `size` box at the origin with `corners`.
SkRRect roundedRect(PixelSize size, const Corners &corners);

/// `box` inset by `widths`, with `corners` inset the same way.
SkRRect insetRoundedRect(PixelSize size, const Corners &corners, const Edges &widths);

/// Paints `images`, CSS gradients as React Native parsed them, over `box`,
/// the last one first. `pointScale` turns a length color stop into pixels.
void drawBackgroundImages(
    SkCanvas &canvas,
    const SkRRect &box,
    const std::vector<facebook::react::BackgroundImage> &images,
    float pointScale,
    float alpha);

} // namespace rnwebos
