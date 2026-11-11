#pragma once

#include "Scene.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"

namespace rnwebos::testing {

/// A scene without a window and the pixels its frames are drawn into, for a
/// test that mounts views, draws them and reads what they painted.
class RasterScene final {
 public:
  explicit RasterScene(PixelSize size) : scene_(size) {
    pixels_.allocPixels(SkImageInfo::Make(size.width, size.height, kRGBA_8888_SkColorType, kPremul_SkAlphaType));
  }

  Scene &scene() {
    return scene_;
  }

  /// Draws the scene's layers over black, as a frame does.
  void draw() {
    SkCanvas canvas(pixels_);
    scene_.drawInto(canvas);
  }

  /// A canvas over the same pixels, for a test that draws without a view.
  SkCanvas canvas() {
    return SkCanvas(pixels_);
  }

  SkColor pixel(int x, int y) const {
    return pixels_.getColor(x, y);
  }

 private:
  Scene scene_;
  SkBitmap pixels_;
};

/// Whether each channel of `actual` is within `tolerance` levels of `expected`.
inline bool nearColor(SkColor actual, SkColor expected, int tolerance = 2) {
  const auto near = [tolerance](unsigned a, unsigned b) {
    return (a > b ? a - b : b - a) <= static_cast<unsigned>(tolerance);
  };
  return near(SkColorGetA(actual), SkColorGetA(expected)) && near(SkColorGetR(actual), SkColorGetR(expected)) &&
         near(SkColorGetG(actual), SkColorGetG(expected)) && near(SkColorGetB(actual), SkColorGetB(expected));
}

} // namespace rnwebos::testing
