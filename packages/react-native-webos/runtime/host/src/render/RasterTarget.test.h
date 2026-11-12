#pragma once

#include "../views/PixelGeometry.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

namespace rnwebos::testing {

/// A CPU surface a test draws on and reads pixels from, transparent at first.
class RasterTarget final {
 public:
  explicit RasterTarget(PixelSize size)
      : surface_(SkSurfaces::Raster(
            SkImageInfo::Make(size.width, size.height, kRGBA_8888_SkColorType, kPremul_SkAlphaType))) {
    canvas().clear(SK_ColorTRANSPARENT);
  }

  SkCanvas &canvas() const {
    return *surface_->getCanvas();
  }

  /// The color of one pixel, its alpha straightened.
  SkColor at(int x, int y) const {
    SkPixmap pixels;
    surface_->peekPixels(&pixels);
    return pixels.getColor(x, y);
  }

 private:
  sk_sp<SkSurface> surface_;
};

} // namespace rnwebos::testing
