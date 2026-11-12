#include "BackdropBlur.h"

#include "../render/RasterScene.test.h"

#include "include/core/SkPaint.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using testing::nearColor;

// A 200x100 frame, white on the left half and black on the right, with
// `blur` drawn over `box`.
struct Blurred {
  Blurred(float sigma, const SkRRect &box, float opacity = 1) {
    auto canvas = frame.canvas();
    canvas.clear(SK_ColorBLACK);
    SkPaint white;
    white.setColor(SK_ColorWHITE);
    canvas.drawRect(SkRect::MakeWH(100, 100), white);
    BackdropBlur blur;
    blur.setSigma(sigma);
    blur.draw(canvas, box, opacity);
  }

  int red(int x, int y) const {
    return static_cast<int>(SkColorGetR(frame.pixel(x, y)));
  }

  testing::RasterScene frame{PixelSize{200, 100}};
};

const auto kBox = SkRRect::MakeRect(SkRect::MakeXYWH(40, 20, 120, 60));

} // namespace

TEST_CASE("a backdrop blur smooths what is under its box and leaves the rest of the frame as it was") {
  const Blurred blurred(8, kBox);

  CHECK(blurred.red(100, 50) > 90);
  CHECK(blurred.red(100, 50) < 165);
  CHECK(blurred.red(92, 50) > blurred.red(100, 50));
  CHECK(blurred.red(108, 50) < blurred.red(100, 50));
  CHECK(blurred.red(108, 50) > 0);
  CHECK(blurred.red(45, 50) > 250);
  CHECK(blurred.red(155, 50) < 5);
  CHECK(blurred.frame.pixel(99, 10) == SK_ColorWHITE);
  CHECK(blurred.frame.pixel(100, 10) == SK_ColorBLACK);
  CHECK(blurred.frame.pixel(100, 85) == SK_ColorBLACK);
}

TEST_CASE("a backdrop blur is cut to its box's rounded corners") {
  const Blurred blurred(8, SkRRect::MakeRectXY(SkRect::MakeXYWH(90, 20, 60, 60), 30, 30));

  CHECK(blurred.frame.pixel(92, 22) == SK_ColorWHITE);
  CHECK(blurred.red(101, 50) > 0);
}

TEST_CASE("a backdrop blur blends in by its opacity, and none or no deviation changes nothing") {
  const Blurred half(8, kBox, 0.5f);
  const Blurred full(8, kBox);
  const Blurred hidden(8, kBox, 0);
  const Blurred sharp(0, kBox);

  CHECK(half.red(102, 50) > 0);
  CHECK(half.red(102, 50) < full.red(102, 50));
  CHECK(hidden.frame.pixel(102, 50) == SK_ColorBLACK);
  CHECK(sharp.frame.pixel(102, 50) == SK_ColorBLACK);
  CHECK(sharp.frame.pixel(98, 50) == SK_ColorWHITE);
}

} // namespace rnwebos
