#pragma once

#include "SvgRootNode.h"
#include "SvgShapeNode.h"

#include "include/core/SkBitmap.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdlib>

namespace rnwebos::svg {

/// A pixel buffer a test draws an `<Svg>` tree into and reads colors back from.
class Raster {
 public:
  Raster(int width, int height) : pixels_(cleared(width, height)), canvas_(pixels_) {}

  SkCanvas &canvas() {
    return canvas_;
  }

  /// Draws `root` as an `<Svg>` filling the buffer would, at one pixel to the point.
  void draw(const RootNode &root) {
    root.renderInto(canvas_, SkSize::Make(pixels_.width(), pixels_.height()), 1);
  }

  SkColor at(int x, int y) const {
    return pixels_.getColor(x, y);
  }

 private:
  static SkBitmap cleared(int width, int height) {
    SkBitmap pixels;
    pixels.allocN32Pixels(width, height);
    pixels.eraseColor(SK_ColorTRANSPARENT);
    return pixels;
  }

  SkBitmap pixels_;
  SkCanvas canvas_;
};

/// A canvas over a 200 x 200 buffer that counts what it is asked to draw.
class DrawProbe final : public SkCanvas {
 public:
  DrawProbe() : SkCanvas(buffer()) {}

  int paths{0};
  int images{0};
  int layers{0};

 protected:
  void onDrawPath(const SkPath &path, const SkPaint &paint) override {
    ++paths;
    SkCanvas::onDrawPath(path, paint);
  }

  void onDrawImageRect2(
      const SkImage *image,
      const SkRect &source,
      const SkRect &destination,
      const SkSamplingOptions &sampling,
      const SkPaint *paint,
      SrcRectConstraint constraint) override {
    ++images;
    SkCanvas::onDrawImageRect2(image, source, destination, sampling, paint, constraint);
  }

  SaveLayerStrategy getSaveLayerStrategy(const SaveLayerRec &layer) override {
    ++layers;
    return SkCanvas::getSaveLayerStrategy(layer);
  }

 private:
  static SkBitmap buffer() {
    SkBitmap pixels;
    pixels.allocN32Pixels(200, 200);
    return pixels;
  }
};

/// Checks a pixel's color, each channel within `tolerance` of `expected`.
inline void checkColor(SkColor actual, SkColor expected, int tolerance = 3) {
  CAPTURE(SkColorGetA(actual));
  CAPTURE(SkColorGetR(actual));
  CAPTURE(SkColorGetG(actual));
  CAPTURE(SkColorGetB(actual));
  CHECK(std::abs(static_cast<int>(SkColorGetA(actual)) - static_cast<int>(SkColorGetA(expected))) <= tolerance);
  CHECK(std::abs(static_cast<int>(SkColorGetR(actual)) - static_cast<int>(SkColorGetR(expected))) <= tolerance);
  CHECK(std::abs(static_cast<int>(SkColorGetG(actual)) - static_cast<int>(SkColorGetG(expected))) <= tolerance);
  CHECK(std::abs(static_cast<int>(SkColorGetB(actual)) - static_cast<int>(SkColorGetB(expected))) <= tolerance);
}

inline RectGeometry box(float x, float y, float width, float height) {
  return {.x = {.value = x}, .y = {.value = y}, .width = {.value = width}, .height = {.value = height}};
}

inline Brush solidBrush(SkColor color) {
  return {.kind = Brush::Kind::Color, .color = SkColor4f::FromColor(color)};
}

/// A paint that lists what it sets, as react-native-svg's `propList` does.
inline Paint filled(SkColor color) {
  Paint paint{.fill = solidBrush(color)};
  paint.listed.set(static_cast<std::size_t>(PaintAttribute::Fill));
  return paint;
}

inline Paint stroked(SkColor color, float width) {
  Paint paint{.fill = {}, .stroke = solidBrush(color), .strokeWidth = {.value = width}};
  paint.listed.set(static_cast<std::size_t>(PaintAttribute::Fill));
  paint.listed.set(static_cast<std::size_t>(PaintAttribute::Stroke));
  paint.listed.set(static_cast<std::size_t>(PaintAttribute::StrokeWidth));
  return paint;
}

} // namespace rnwebos::svg
