#pragma once

#include "PixelGeometry.h"

#include "include/core/SkRect.h"
#include "include/core/SkSize.h"

namespace rnwebos {

/// How an image fills its box: React Native's `resizeMode` and expo-image's
/// `contentFit` in one vocabulary.
enum class ImageFit {
  /// Scaled evenly to cover the box, centered, the overflow cut.
  Cover,
  /// Scaled evenly to fit inside the box, centered.
  Contain,
  /// Stretched to the box.
  Stretch,
  /// At its natural size, centered, scaled down only to fit inside the box.
  Center,
  /// At its natural size, centered, whatever the box.
  Natural,
  /// At its natural size, tiled from the box's corner.
  Repeat,
};

/// Where an image is painted in its view.
struct ImagePlacement {
  /// The rectangle painted, in the view's pixels.
  SkRect destination;
  /// The part of the image painted there, in the image's own pixels.
  SkRect source;
  /// With `Repeat`, the size of one copy in the view's pixels; empty otherwise.
  SkSize tile;

  bool operator==(const ImagePlacement &) const = default;
};

/// Places an image of `image` pixels, drawn at `naturalScale` view pixels per
/// image pixel when unscaled, in `box` for `fit`.
ImagePlacement placeImage(ImageFit fit, const PixelRect &box, PixelSize image, float naturalScale);

/// The size worth decoding an image of `image` pixels to for a `box` and
/// `fit`: the size it is drawn at when it is at least twice that, so a large
/// picture in a small box costs neither memory nor a lazy downscale at every
/// frame, and its own size otherwise.
PixelSize decodedImageSize(ImageFit fit, PixelSize box, PixelSize image, float naturalScale);

} // namespace rnwebos
