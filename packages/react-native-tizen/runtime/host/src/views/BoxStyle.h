#pragma once

#include "NinePatch.h"
#include "PatternedRing.h"

#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/LayoutMetrics.h>

#include <vector>

namespace rntizen {

/// One drop shadow, in pixels: a CSS `boxShadow` entry, or the iOS
/// `shadowColor`/`shadowOffset`/`shadowOpacity`/`shadowRadius` props as one.
struct ShadowStyle {
  int offsetX{0};
  int offsetY{0};
  int blur{0};
  int spread{0};
  facebook::react::SharedColor color;
  float opacity{1};

  bool operator==(const ShadowStyle &) const = default;
};

/// A ring drawn `offset` pixels outside the border box (inside when negative).
struct OutlineStyle {
  int width{0};
  int offset{0};
  facebook::react::SharedColor color;

  bool operator==(const OutlineStyle &) const = default;
};

/// What a view's box draws, resolved from its props and layout in device pixels.
struct BoxStyle {
  facebook::react::SharedColor backgroundColor;
  Corners corners;
  Edges borderWidths;
  facebook::react::BorderColors borderColors;
  StrokePattern borderPattern{StrokePattern::Solid};
  OutlineStyle outline;
  std::vector<ShadowStyle> shadows;
  std::vector<facebook::react::BackgroundImage> backgroundImages;
  facebook::react::Float pointScale{1};
  bool clips{false};

  bool operator==(const BoxStyle &) const = default;
};

/// The box of a view with `props` laid out as `metrics`. `forceClip` clips
/// content to the box even when `overflow` is visible, as an image does.
BoxStyle resolveBoxStyle(
    const facebook::react::ViewProps &props,
    const facebook::react::LayoutMetrics &metrics,
    bool forceClip);

} // namespace rntizen
