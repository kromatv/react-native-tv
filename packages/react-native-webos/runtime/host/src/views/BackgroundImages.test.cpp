#include "BoxPainter.h"
#include "GradientShader.h"

#include "../render/RasterScene.test.h"

#include <doctest/doctest.h>

#include <vector>

namespace rnwebos {

namespace {

using facebook::react::BackgroundImage;
using facebook::react::ColorStop;
using facebook::react::colorFromRGBA;
using facebook::react::LinearGradient;
using facebook::react::RadialGradient;
using facebook::react::RadialGradientShape;
using facebook::react::RadialGradientSize;
using facebook::react::UnitType;
using facebook::react::ValueUnit;
using testing::nearColor;

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);
// What `processBackgroundImage.webos.ts` sends for `transparent`.
const auto kClear = colorFromRGBA(0, 0, 1, 0);

ColorStop stop(facebook::react::SharedColor color, ValueUnit position = {}) {
  return {.color = color, .position = position};
}

ValueUnit percent(float value) {
  return {value, UnitType::Percent};
}

BackgroundImage toRight(std::vector<ColorStop> stops) {
  return LinearGradient{.direction = 90.0f, .colorStops = std::move(stops)};
}

BackgroundImage circle(RadialGradientSize::SizeKeyword size, std::vector<ColorStop> stops) {
  RadialGradient gradient;
  gradient.shape = RadialGradientShape::Circle;
  gradient.size = {.value = size};
  gradient.colorStops = std::move(stops);
  return gradient;
}

// A 200x100 box at (20, 10) of a black 240x120 frame, painted with `images`.
class Painted final {
 public:
  explicit Painted(const std::vector<BackgroundImage> &images, float radius = 0, float alpha = 1) {
    auto canvas = frame_.canvas();
    canvas.clear(SK_ColorBLACK);
    drawBackgroundImages(canvas, SkRRect::MakeRectXY(SkRect::MakeXYWH(20, 10, 200, 100), radius, radius), images, 1, alpha);
  }

  // The color at (`x`, `y`) of the box.
  SkColor at(int x, int y) const {
    return frame_.pixel(20 + x, 10 + y);
  }

 private:
  testing::RasterScene frame_{PixelSize{240, 120}};
};

} // namespace

TEST_CASE("a linear gradient runs from its first color to its last across the box and nowhere else") {
  const Painted painted({toRight({stop(kRed), stop(kBlue)})});

  CHECK(nearColor(painted.at(0, 50), SkColorSetRGB(254, 0, 1)));
  CHECK(nearColor(painted.at(100, 50), SkColorSetRGB(127, 0, 128)));
  CHECK(nearColor(painted.at(199, 5), SkColorSetRGB(1, 0, 254)));
  CHECK(painted.at(-1, 50) == SK_ColorBLACK);
  CHECK(painted.at(200, 50) == SK_ColorBLACK);
}

TEST_CASE("a gradient holds its end colors outside its stops and jumps at a hard stop") {
  const Painted painted({toRight({stop(kRed, percent(25)), stop(kRed, percent(50)), stop(kBlue, percent(50))})});

  CHECK(nearColor(painted.at(10, 50), SK_ColorRED));
  CHECK(nearColor(painted.at(98, 50), SK_ColorRED));
  CHECK(nearColor(painted.at(102, 50), SK_ColorBLUE));
  CHECK(nearColor(painted.at(190, 50), SK_ColorBLUE));
}

TEST_CASE("a gradient to transparent fades its color out without turning it toward black first") {
  const Painted painted({toRight({stop(kRed), stop(kClear)})});

  CHECK(nearColor(painted.at(100, 50), SkColorSetRGB(127, 0, 0)));
  CHECK(nearColor(painted.at(150, 50), SkColorSetRGB(63, 0, 0)));
}

TEST_CASE("stops placed before and after the line show only the part of the gradient the box covers") {
  const Painted painted({toRight({stop(kRed, percent(-50)), stop(kBlue, percent(150))})});

  CHECK(nearColor(painted.at(0, 50), SkColorSetRGB(191, 0, 64)));
  CHECK(nearColor(painted.at(199, 50), SkColorSetRGB(64, 0, 191)));
}

TEST_CASE("a transition hint moves the middle of the transition to the hint") {
  const Painted painted({toRight({stop(kRed, percent(0)), stop({}, percent(25)), stop(kBlue, percent(100))})});

  CHECK(nearColor(painted.at(50, 50), SkColorSetRGB(127, 0, 128)));
  CHECK(nearColor(painted.at(100, 50), SkColorSetRGB(74, 0, 181)));
}

TEST_CASE("a closest-side circle ends on the nearer sides and holds its last color beyond") {
  const Painted painted({circle(RadialGradientSize::SizeKeyword::ClosestSide, {stop(kRed), stop(kBlue)})});

  CHECK(nearColor(painted.at(100, 50), SK_ColorRED, 6));
  CHECK(nearColor(painted.at(125, 50), SkColorSetRGB(127, 0, 128), 4));
  CHECK(nearColor(painted.at(100, 25), SkColorSetRGB(127, 0, 128), 4));
  CHECK(nearColor(painted.at(160, 50), SK_ColorBLUE));
  CHECK(nearColor(painted.at(5, 5), SK_ColorBLUE));
}

TEST_CASE("a radial gradient keeps going to a stop past its ending shape") {
  const Painted painted(
      {circle(RadialGradientSize::SizeKeyword::ClosestSide, {stop(kRed, percent(0)), stop(kBlue, percent(200))})});

  CHECK(nearColor(painted.at(150, 50), SkColorSetRGB(127, 0, 128), 4));
  CHECK(nearColor(painted.at(199, 50), SkColorSetRGB(2, 0, 253), 4));
}

TEST_CASE("an ellipse's radii follow the box's sides") {
  RadialGradient gradient;
  gradient.shape = RadialGradientShape::Ellipse;
  gradient.size = {.value = RadialGradientSize::SizeKeyword::ClosestSide};
  gradient.colorStops = {stop(kRed), stop(kBlue)};
  const Painted painted({gradient});

  CHECK(nearColor(painted.at(150, 50), SkColorSetRGB(127, 0, 128), 4));
  CHECK(nearColor(painted.at(100, 75), SkColorSetRGB(127, 0, 128), 6));
}

TEST_CASE("the first background image is painted over the ones after it") {
  const Painted painted({toRight({stop(kRed, percent(0)), stop(kClear, percent(100))}), toRight({stop(kBlue), stop(kBlue)})});

  CHECK(nearColor(painted.at(1, 50), SkColorSetRGB(253, 0, 2)));
  CHECK(nearColor(painted.at(198, 50), SkColorSetRGB(2, 0, 253)));
}

TEST_CASE("a gradient is cut to the box's rounded corners and takes the box's opacity") {
  const Painted rounded({toRight({stop(kRed), stop(kRed)})}, 40);
  const Painted faded({toRight({stop(kRed), stop(kRed)})}, 0, 0.5f);

  CHECK(rounded.at(2, 2) == SK_ColorBLACK);
  CHECK(nearColor(rounded.at(40, 40), SK_ColorRED));
  CHECK(nearColor(faded.at(100, 50), SkColorSetRGB(128, 0, 0)));
}

TEST_CASE("gradientShader gives the same shader for the same gradient and box, and none without colors or area") {
  const auto image = toRight({stop(kRed), stop(kBlue)});

  const auto first = gradientShader(image, 200, 100, 1);
  CHECK(first != nullptr);
  CHECK(gradientShader(image, 200, 100, 1) == first);
  CHECK(gradientShader(image, 100, 100, 1) != first);
  CHECK(gradientShader(toRight({stop(kBlue), stop(kRed)}), 200, 100, 1) != first);
  CHECK(gradientShader(toRight({}), 200, 100, 1) == nullptr);
  CHECK(gradientShader(image, 0, 100, 1) == nullptr);
}

} // namespace rnwebos
