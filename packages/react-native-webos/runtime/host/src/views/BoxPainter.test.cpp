#include "BoxPainter.h"

#include "../render/RasterTarget.test.h"

#include <doctest/doctest.h>

#include <react/renderer/graphics/Color.h>

namespace rnwebos {

namespace {

using facebook::react::colorFromRGBA;
using testing::RasterTarget;

constexpr PixelSize kCanvas{.width = 200, .height = 200};
constexpr PixelSize kBox{.width = 100, .height = 100};
constexpr float kBoxAt = 50;

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kGreen = colorFromRGBA(0, 255, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);
const auto kYellow = colorFromRGBA(255, 255, 0, 255);

/// A box drawn whole at (50, 50) of a transparent canvas; pixels are read in the box's own coordinates.
class DrawnBox final {
 public:
  explicit DrawnBox(const BoxStyle &style, float alpha = 1) {
    painter_.update(style);
    target_.canvas().translate(kBoxAt, kBoxAt);
    painter_.drawBelowContent(target_.canvas(), kBox, alpha);
    painter_.drawAboveChildren(target_.canvas(), kBox, alpha);
  }

  SkColor at(int x, int y) const {
    return target_.at(x + static_cast<int>(kBoxAt), y + static_cast<int>(kBoxAt));
  }

  const BoxPainter &painter() const {
    return painter_;
  }

 private:
  BoxPainter painter_;
  RasterTarget target_{kCanvas};
};

} // namespace

TEST_CASE("BoxPainter fills the box with its background, and nothing outside it") {
  const DrawnBox box({.backgroundColor = kRed});

  CHECK(box.at(0, 0) == SK_ColorRED);
  CHECK(box.at(99, 99) == SK_ColorRED);
  CHECK(box.at(-1, 50) == SK_ColorTRANSPARENT);
  CHECK(box.at(100, 50) == SK_ColorTRANSPARENT);
  CHECK(box.painter().layerCount() == 1);
  CHECK(box.painter().overflow() == Edges{});
}

TEST_CASE("BoxPainter rounds each corner by its own radii, elliptical ones included") {
  const DrawnBox box({
      .backgroundColor = kRed,
      .corners = {.topLeft = {.x = 20, .y = 20}, .topRight = {.x = 40, .y = 10}, .bottomRight = {}, .bottomLeft = {.x = 50, .y = 50}},
  });

  CHECK(box.at(2, 2) == SK_ColorTRANSPARENT);
  CHECK(box.at(10, 10) == SK_ColorRED);
  CHECK(box.at(97, 3) == SK_ColorTRANSPARENT);
  CHECK(box.at(70, 2) == SK_ColorRED);
  CHECK(box.at(97, 12) == SK_ColorRED);
  CHECK(box.at(99, 99) == SK_ColorRED);
  CHECK(box.at(10, 90) == SK_ColorTRANSPARENT);
  CHECK(box.at(20, 80) == SK_ColorRED);
  CHECK(box.at(50, 50) == SK_ColorRED);
}

TEST_CASE("BoxPainter multiplies what it draws by the alpha it is given") {
  const DrawnBox box({.backgroundColor = kRed}, 0.5f);

  CHECK(SkColorGetA(box.at(50, 50)) == doctest::Approx(128).epsilon(0.02));
  CHECK(SkColorGetR(box.at(50, 50)) == 255);
}

TEST_CASE("BoxPainter draws a border of one color as a ring of each side's width") {
  const DrawnBox box({
      .borderWidths = {.left = 10, .top = 4, .right = 0, .bottom = 6},
      .borderColors = {.left = kBlue, .top = kBlue, .right = kBlue, .bottom = kBlue},
  });

  CHECK(box.at(5, 50) == SK_ColorBLUE);
  CHECK(box.at(9, 50) == SK_ColorBLUE);
  CHECK(box.at(10, 50) == SK_ColorTRANSPARENT);
  CHECK(box.at(50, 3) == SK_ColorBLUE);
  CHECK(box.at(50, 4) == SK_ColorTRANSPARENT);
  CHECK(box.at(99, 50) == SK_ColorTRANSPARENT);
  CHECK(box.at(50, 93) == SK_ColorTRANSPARENT);
  CHECK(box.at(50, 94) == SK_ColorBLUE);
  CHECK(box.painter().layerCount() == 1);
}

TEST_CASE("BoxPainter gives each side its own color, the corners cut along their diagonals") {
  const DrawnBox box({
      .borderWidths = {.left = 10, .top = 10, .right = 10, .bottom = 10},
      .borderColors = {.left = kRed, .top = kGreen, .right = kBlue, .bottom = kYellow},
  });

  CHECK(box.at(5, 50) == SK_ColorRED);
  CHECK(box.at(50, 5) == SK_ColorGREEN);
  CHECK(box.at(94, 50) == SK_ColorBLUE);
  CHECK(box.at(50, 94) == SK_ColorYELLOW);
  CHECK(box.at(2, 7) == SK_ColorRED);
  CHECK(box.at(7, 2) == SK_ColorGREEN);
  CHECK(box.at(97, 92) == SK_ColorBLUE);
  CHECK(box.at(92, 97) == SK_ColorYELLOW);
  CHECK(box.at(50, 50) == SK_ColorTRANSPARENT);
}

TEST_CASE("BoxPainter skips a side whose color is unset") {
  const DrawnBox box({
      .borderWidths = {.left = 10, .top = 10, .right = 10, .bottom = 10},
      .borderColors = {.left = kRed, .top = {}, .right = kRed, .bottom = kRed},
  });

  CHECK(box.at(5, 50) == SK_ColorRED);
  CHECK(box.at(50, 5) == SK_ColorTRANSPARENT);
}

TEST_CASE("BoxPainter rounds a border's inside by its radius less its width") {
  const DrawnBox box({
      .corners = {.topLeft = {.x = 30, .y = 30}, .topRight = {}, .bottomRight = {}, .bottomLeft = {}},
      .borderWidths = {.left = 10, .top = 10, .right = 10, .bottom = 10},
      .borderColors = {.left = kBlue, .top = kBlue, .right = kBlue, .bottom = kBlue},
  });

  CHECK(box.at(3, 3) == SK_ColorTRANSPARENT);
  CHECK(box.at(12, 12) == SK_ColorBLUE);
  CHECK(box.at(14, 14) == SK_ColorBLUE);
  CHECK(box.at(22, 22) == SK_ColorTRANSPARENT);
  CHECK(box.at(89, 11) == SK_ColorTRANSPARENT);
  CHECK(box.at(91, 9) == SK_ColorBLUE);
}

TEST_CASE("BoxPainter strokes a dashed border in dashes with gaps between them") {
  const DrawnBox box({
      .borderWidths = {.left = 4, .top = 4, .right = 4, .bottom = 4},
      .borderColors = {.left = kBlue, .top = kBlue, .right = kBlue, .bottom = kBlue},
      .borderPattern = StrokePattern::Dashed,
  });
  int painted = 0;
  int clear = 0;
  for (int x = 10; x < 90; ++x) {
    const auto color = box.at(x, 2);
    if (color == SK_ColorBLUE) ++painted;
    if (color == SK_ColorTRANSPARENT) ++clear;
  }

  CHECK(painted > 20);
  CHECK(clear > 10);
  CHECK(box.at(50, 50) == SK_ColorTRANSPARENT);
}

TEST_CASE("BoxPainter draws a sharp shadow at its offset, grown by its spread, under an opaque box") {
  const DrawnBox box({
      .backgroundColor = kRed,
      .shadows = {{.offsetX = 10, .offsetY = 20, .blur = 0, .spread = 5, .color = kBlue, .opacity = 1}},
  });

  CHECK(box.at(50, 50) == SK_ColorRED);
  CHECK(box.at(112, 50) == SK_ColorBLUE);
  CHECK(box.at(114, 50) == SK_ColorBLUE);
  CHECK(box.at(116, 50) == SK_ColorTRANSPARENT);
  CHECK(box.at(50, 124) == SK_ColorBLUE);
  CHECK(box.at(50, 126) == SK_ColorTRANSPARENT);
  CHECK(box.at(-1, 50) == SK_ColorTRANSPARENT);
  CHECK(box.painter().overflow() == Edges{.left = 0, .top = 0, .right = 15, .bottom = 25});
  CHECK(box.painter().layerCount() == 2);
}

TEST_CASE("BoxPainter draws no shadow under a box that lets it show through") {
  const DrawnBox translucent({
      .backgroundColor = colorFromRGBA(255, 0, 0, 128),
      .shadows = {{.offsetX = 0, .offsetY = 0, .blur = 0, .spread = 10, .color = kBlue, .opacity = 1}},
  });
  const DrawnBox empty({
      .shadows = {{.offsetX = 0, .offsetY = 0, .blur = 0, .spread = 10, .color = kBlue, .opacity = 1}},
  });

  CHECK(SkColorGetB(translucent.at(50, 50)) == 0);
  CHECK(translucent.at(-5, 50) == SK_ColorBLUE);
  CHECK(empty.at(50, 50) == SK_ColorTRANSPARENT);
  CHECK(empty.at(105, 50) == SK_ColorBLUE);
}

TEST_CASE("BoxPainter blurs a shadow, fading with distance, at the shadow's opacity") {
  const DrawnBox box({
      .backgroundColor = kRed,
      .shadows = {{.offsetX = 0, .offsetY = 0, .blur = 20, .spread = 0, .color = kBlue, .opacity = 0.5f}},
  });
  const auto near = SkColorGetA(box.at(102, 50));
  const auto far = SkColorGetA(box.at(115, 50));

  CHECK(near > far);
  CHECK(far > 0);
  CHECK(near < 128);
  CHECK(box.at(135, 50) == SK_ColorTRANSPARENT);
  CHECK(box.painter().overflow() == Edges{.left = 30, .top = 30, .right = 30, .bottom = 30});
}

TEST_CASE("BoxPainter draws the outline as a ring outside the border box, at its offset") {
  const DrawnBox box({
      .backgroundColor = kRed,
      .outline = {.width = 4, .offset = 6, .color = kGreen},
  });

  CHECK(box.at(50, 50) == SK_ColorRED);
  CHECK(box.at(103, 50) == SK_ColorTRANSPARENT);
  CHECK(box.at(107, 50) == SK_ColorGREEN);
  CHECK(box.at(108, 50) == SK_ColorGREEN);
  CHECK(box.at(111, 50) == SK_ColorTRANSPARENT);
  CHECK(box.at(50, -8) == SK_ColorGREEN);
  CHECK(box.painter().overflow() == Edges{.left = 10, .top = 10, .right = 10, .bottom = 10});
  CHECK(box.painter().layerCount() == 2);
}

TEST_CASE("BoxPainter clips content to the padding box, rounded with it") {
  BoxPainter painter;
  painter.update({
      .corners = {.topLeft = {.x = 30, .y = 30}, .topRight = {}, .bottomRight = {}, .bottomLeft = {}},
      .borderWidths = {.left = 10, .top = 10, .right = 10, .bottom = 10},
      .clips = true,
  });
  RasterTarget target(kCanvas);

  painter.clipContent(target.canvas(), kBox);
  target.canvas().drawColor(SK_ColorRED);

  CHECK(target.at(5, 50) == SK_ColorTRANSPARENT);
  CHECK(target.at(12, 12) == SK_ColorTRANSPARENT);
  CHECK(target.at(20, 20) == SK_ColorRED);
  CHECK(target.at(50, 50) == SK_ColorRED);
  CHECK(target.at(89, 11) == SK_ColorRED);
  CHECK(target.at(91, 50) == SK_ColorTRANSPARENT);
  CHECK(target.at(150, 150) == SK_ColorTRANSPARENT);
}

TEST_CASE("BoxPainter says whether an update changed its style") {
  BoxPainter painter;
  const BoxStyle style{.backgroundColor = kRed};

  CHECK(painter.update(style));
  CHECK_FALSE(painter.update(style));
  CHECK(painter.update({.backgroundColor = kBlue}));
  CHECK(painter.style().backgroundColor == kBlue);
}

} // namespace rnwebos
