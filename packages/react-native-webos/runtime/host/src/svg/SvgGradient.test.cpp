#include "SvgGradient.h"

#include "SvgGradientNode.h"
#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor4f kRed{1, 0, 0, 1};
constexpr SkColor4f kBlue{0, 0, 1, 1};

GradientDefinition redToBlue() {
  return {
      .x2 = {.value = 1},
      .stops = {{.offset = 0, .color = kRed}, {.offset = 1, .color = kBlue}},
  };
}

bool isShader(const GradientPaint &paint) {
  return std::holds_alternative<sk_sp<SkShader>>(paint);
}

/// A 100 x 40 rectangle at the origin, filled with the gradient named `ramp`.
struct FilledRect {
  RootNode root;
  GradientNode gradient;
  RectNode rect;

  explicit FilledRect(GradientDefinition definition) {
    gradient.name = "ramp";
    gradient.definition = std::move(definition);
    rect.setGeometry({.width = {.value = 100}, .height = {.value = 40}});
    rect.paint.fill = {.kind = Brush::Kind::Reference, .reference = "ramp"};
    root.setChildren({&gradient, &rect});
  }
};

} // namespace

TEST_CASE("gradientStops reads offset and ARGB pairs, signed as JavaScript's bit operations leave them") {
  const auto stops = gradientStops({0, -65536.0F, 1, -16776961.0F});

  REQUIRE(stops.size() == 2);
  CHECK(stops[0].offset == doctest::Approx(0));
  CHECK(stops[0].color == kRed);
  CHECK(stops[1].offset == doctest::Approx(1));
  CHECK(stops[1].color == kBlue);
}

TEST_CASE("gradientStops keeps offsets between 0 and 1 and never decreasing") {
  const auto stops = gradientStops({-0.5F, 0, 0.6F, 0, 0.4F, 0, 3, 0});

  REQUIRE(stops.size() == 4);
  CHECK(stops[0].offset == doctest::Approx(0));
  CHECK(stops[2].offset == doctest::Approx(0.6F));
  CHECK(stops[3].offset == doctest::Approx(1));
}

TEST_CASE("gradientStops drops a trailing offset without a color") {
  CHECK(gradientStops({0, 0, 1}).size() == 1);
}

TEST_CASE("a gradient without stops paints nothing") {
  GradientDefinition definition;

  CHECK(std::holds_alternative<std::monostate>(gradientPaint(definition, SkRect::MakeWH(10, 10), {10, 10})));
}

TEST_CASE("a gradient with one stop paints its color") {
  GradientDefinition definition{.stops = {{.offset = 0.5F, .color = kBlue}}};

  const auto paint = gradientPaint(definition, SkRect::MakeWH(10, 10), {10, 10});

  REQUIRE(std::holds_alternative<SkColor4f>(paint));
  CHECK(std::get<SkColor4f>(paint) == kBlue);
}

TEST_CASE("a linear gradient of no length paints its last stop's color") {
  auto definition = redToBlue();
  definition.x2 = {};

  const auto paint = gradientPaint(definition, SkRect::MakeWH(10, 10), {10, 10});

  REQUIRE(std::holds_alternative<SkColor4f>(paint));
  CHECK(std::get<SkColor4f>(paint) == kBlue);
}

TEST_CASE("a radial gradient without a radius paints its last stop's color") {
  auto definition = redToBlue();
  definition.radial = true;

  CHECK(std::holds_alternative<SkColor4f>(gradientPaint(definition, SkRect::MakeWH(10, 10), {10, 10})));
}

TEST_CASE("a bounding-box gradient paints nothing on a shape without width or height") {
  CHECK(std::holds_alternative<std::monostate>(gradientPaint(redToBlue(), SkRect::MakeWH(10, 0), {10, 10})));
  CHECK(isShader(gradientPaint(redToBlue(), SkRect::MakeWH(10, 10), {10, 10})));
}

TEST_CASE("a user-space gradient needs no bounding box") {
  auto definition = redToBlue();
  definition.userSpace = true;

  CHECK(isShader(gradientPaint(definition, SkRect::MakeWH(10, 0), {10, 10})));
}

TEST_CASE("a gradient whose transform cannot be inverted paints its last stop's color") {
  auto definition = redToBlue();
  definition.transform = SkMatrix::Scale(0, 1);

  CHECK(std::holds_alternative<SkColor4f>(gradientPaint(definition, SkRect::MakeWH(10, 10), {10, 10})));
}

TEST_CASE("a bounding-box linear gradient runs across the shape it fills") {
  FilledRect scene(redToBlue());
  Raster raster(100, 40);

  raster.draw(scene.root);

  checkColor(raster.at(1, 20), SkColorSetRGB(252, 0, 3), 4);
  checkColor(raster.at(50, 20), SkColorSetRGB(127, 0, 128), 4);
  checkColor(raster.at(98, 20), SkColorSetRGB(3, 0, 252), 4);
}

TEST_CASE("a user-space linear gradient runs in user units, percentages against the viewport") {
  auto definition = redToBlue();
  definition.userSpace = true;
  definition.x1 = {.value = 50, .percent = true};
  definition.x2 = {.value = 100, .percent = true};
  FilledRect scene(definition);
  Raster raster(100, 40);

  raster.draw(scene.root);

  checkColor(raster.at(25, 20), SK_ColorRED);
  checkColor(raster.at(75, 20), SkColorSetRGB(127, 0, 128), 4);
}

TEST_CASE("a gradient transform applies inside the gradient's own space") {
  auto definition = redToBlue();
  definition.transform = SkMatrix::MakeAll(0, -1, 1, 1, 0, 0, 0, 0, 1);
  FilledRect scene(definition);
  Raster raster(100, 40);

  raster.draw(scene.root);

  checkColor(raster.at(50, 1), SkColorSetRGB(245, 0, 10), 4);
  checkColor(raster.at(50, 38), SkColorSetRGB(10, 0, 245), 4);
}

TEST_CASE("a stop's alpha fades the fill, without darkening it") {
  auto definition = redToBlue();
  definition.stops[1].color = {1, 0, 0, 0};
  FilledRect scene(definition);
  Raster raster(100, 40);

  raster.draw(scene.root);

  checkColor(raster.at(50, 20), SkColorSetARGB(128, 255, 0, 0), 4);
}

TEST_CASE("a bounding-box radial gradient is an ellipse in a shape that is no square") {
  GradientDefinition definition{
      .radial = true,
      .cx = {.value = 0.5F},
      .cy = {.value = 0.5F},
      .rx = {.value = 0.5F},
      .ry = {.value = 0.5F},
      .fx = {.value = 0.5F},
      .fy = {.value = 0.5F},
      .stops = {{.offset = 0, .color = kRed}, {.offset = 1, .color = kBlue}},
  };
  FilledRect scene(definition);
  Raster raster(100, 40);

  raster.draw(scene.root);

  checkColor(raster.at(50, 20), SK_ColorRED, 8);
  checkColor(raster.at(0, 20), SK_ColorBLUE, 8);
  checkColor(raster.at(50, 0), SK_ColorBLUE, 8);
  checkColor(raster.at(75, 20), SkColorSetRGB(127, 0, 128), 6);
}

TEST_CASE("a radial gradient's focal point moves where the first stop sits") {
  GradientDefinition definition{
      .radial = true,
      .cx = {.value = 0.5F},
      .cy = {.value = 0.5F},
      .rx = {.value = 0.5F},
      .ry = {.value = 0.5F},
      .fx = {.value = 0.25F},
      .fy = {.value = 0.5F},
      .stops = {{.offset = 0, .color = kRed}, {.offset = 1, .color = kBlue}},
  };
  FilledRect scene(definition);
  Raster raster(100, 40);

  raster.draw(scene.root);

  checkColor(raster.at(25, 20), SK_ColorRED, 8);
  CHECK(SkColorGetB(raster.at(50, 20)) > 60);
}

TEST_CASE("a fill naming no gradient paints nothing") {
  FilledRect scene(redToBlue());
  scene.rect.paint.fill.reference = "missing";
  Raster raster(100, 40);

  raster.draw(scene.root);

  CHECK(raster.at(50, 20) == SK_ColorTRANSPARENT);
}

} // namespace rnwebos::svg
