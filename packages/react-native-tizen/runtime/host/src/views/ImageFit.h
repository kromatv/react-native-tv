#pragma once

#include "PixelGeometry.h"

#include <react/renderer/imagemanager/primitives.h>

namespace rntizen {

/// Where an image is drawn inside its view: the image object's geometry and
/// its fill, the rectangle one copy of the image covers relative to that
/// geometry (Evas tiles the fill across the object).
struct ImagePlacement {
  PixelRect geometry;
  PixelRect fill;
};

/// Places an image of `imageWidth` x `imageHeight` pixels, drawn at
/// `naturalScale` pixels per image pixel when unscaled, in `box` for `resizeMode`.
ImagePlacement placeImage(
    facebook::react::ImageResizeMode resizeMode,
    const PixelRect &box,
    int imageWidth,
    int imageHeight,
    float naturalScale);

} // namespace rntizen
