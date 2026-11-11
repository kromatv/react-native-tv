#include "SvgPatternNode.h"

#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;

/// A 40 x 40 rectangle at 10, 10 filled with the pattern `tiles`, whose one
/// child is a red square in the top-left quarter of the unit the test picks.
struct Patterned {
  RootNode root;
  PatternNode pattern;
  RectNode mark;
  RectNode rect;

  explicit Patterned(RectGeometry markGeometry) {
    pattern.name = "tiles";
    mark.setGeometry(markGeometry);
    mark.paint = filled(SK_ColorRED);
    pattern.setChildren({&mark});
    rect.setGeometry(box(10, 10, 40, 40));
    rect.paint.fill = {.kind = Brush::Kind::Reference, .reference = "tiles"};
    root.setChildren({&pattern, &rect});
  }
};

} // namespace

TEST_CASE("a pattern's tile is a share of the painted box and repeats from its corner") {
  Patterned scene(box(0, 0, 10, 10));
  scene.pattern.width = {.value = 0.5F};
  scene.pattern.height = {.value = 50, .percent = true};
  Raster raster(60, 60);

  raster.draw(scene.root);

  checkColor(raster.at(15, 15), SK_ColorRED);
  checkColor(raster.at(35, 15), SK_ColorRED);
  checkColor(raster.at(15, 35), SK_ColorRED);
  CHECK(raster.at(25, 15) == kClear);
  CHECK(raster.at(15, 25) == kClear);
  CHECK(raster.at(5, 5) == kClear);
}

TEST_CASE("a pattern's tile in user space starts at its own x and y") {
  Patterned scene(box(0, 0, 5, 5));
  scene.pattern.tileInUserSpace = true;
  scene.pattern.x = {.value = 5};
  scene.pattern.y = {.value = 5};
  scene.pattern.width = {.value = 10};
  scene.pattern.height = {.value = 10};
  Raster raster(60, 60);

  raster.draw(scene.root);

  checkColor(raster.at(17, 17), SK_ColorRED);
  checkColor(raster.at(27, 37), SK_ColorRED);
  CHECK(raster.at(12, 12) == kClear);
  CHECK(raster.at(22, 22) == kClear);
}

TEST_CASE("a pattern's content can be laid out in the painted box, or in a viewBox") {
  Patterned inBox(box(0, 0, 0.25F, 0.25F));
  inBox.pattern.contentInBoundingBox = true;
  inBox.pattern.width = {.value = 0.5F};
  inBox.pattern.height = {.value = 0.5F};
  Raster first(60, 60);
  first.draw(inBox.root);
  checkColor(first.at(15, 15), SK_ColorRED);
  CHECK(first.at(25, 25) == kClear);

  Patterned inViewBox(box(0, 0, 1, 1));
  inViewBox.pattern.width = {.value = 0.5F};
  inViewBox.pattern.height = {.value = 0.5F};
  inViewBox.pattern.viewBox = {.width = 2, .height = 2, .align = "xMidYMid"};
  Raster second(60, 60);
  second.draw(inViewBox.root);
  checkColor(second.at(15, 15), SK_ColorRED);
  CHECK(second.at(25, 25) == kClear);
}

TEST_CASE("a pattern transform applies to the tiles") {
  Patterned scene(box(0, 0, 10, 10));
  scene.pattern.width = {.value = 0.5F};
  scene.pattern.height = {.value = 0.5F};
  scene.pattern.transform = SkMatrix::Translate(10, 0);
  Raster raster(60, 60);

  raster.draw(scene.root);

  checkColor(raster.at(25, 15), SK_ColorRED);
  CHECK(raster.at(15, 15) == kClear);
}

TEST_CASE("a pattern without a tile, or one that paints with itself, paints nothing") {
  Patterned empty(box(0, 0, 10, 10));
  empty.pattern.width = {};
  Raster first(60, 60);
  first.draw(empty.root);
  CHECK(first.at(15, 15) == kClear);

  Patterned loop(box(0, 0, 10, 10));
  loop.pattern.width = {.value = 0.5F};
  loop.pattern.height = {.value = 0.5F};
  loop.mark.paint.fill = {.kind = Brush::Kind::Reference, .reference = "tiles"};
  Raster second(60, 60);
  second.draw(loop.root);
  CHECK(second.at(15, 15) == kClear);
}

} // namespace rnwebos::svg
