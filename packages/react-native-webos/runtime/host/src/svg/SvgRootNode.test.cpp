#include "SvgRootNode.h"

#include "SvgClipPathNode.h"
#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;

/// A node that counts how often the tree walks it.
class CountingNode final : public Node {
 public:
  mutable int drawn{0};

 protected:
  void draw(SkCanvas &, const RenderContext &) const override {
    ++drawn;
  }
};

} // namespace

TEST_CASE("a stroke scales with the transform it is drawn through") {
  RootNode root;
  root.viewBox = {.width = 10, .height = 10, .align = "xMidYMid"};
  LineNode line;
  line.setGeometry({.x1 = {.value = 0}, .y1 = {.value = 5}, .x2 = {.value = 10}, .y2 = {.value = 5}});
  line.paint = stroked(SK_ColorWHITE, 2);
  root.setChildren({&line});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 17), SK_ColorWHITE);
  checkColor(raster.at(20, 22), SK_ColorWHITE);
  CHECK(raster.at(20, 14) == kClear);
}

TEST_CASE("percentages resolve against the viewBox when there is one") {
  RootNode root;
  root.viewBox = {.width = 10, .height = 10, .align = "xMidYMid"};
  RectNode rect;
  rect.setGeometry({.width = {.value = 50, .percent = true}, .height = {.value = 100, .percent = true}});
  root.setChildren({&rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(10, 20), SK_ColorBLACK);
  CHECK(raster.at(30, 20) == kClear);
}

TEST_CASE("the scale turns layout points into pixels") {
  RootNode root;
  RectNode rect;
  rect.setGeometry(box(0, 0, 10, 10));
  root.setChildren({&rect});
  Raster raster(40, 40);

  root.renderInto(raster.canvas(), {20, 20}, 2);

  checkColor(raster.at(18, 18), SK_ColorBLACK);
  CHECK(raster.at(22, 22) == kClear);
}

TEST_CASE("a tree is walked once and played again until a node reports a change") {
  RootNode root;
  CountingNode shape;
  root.setChildren({&shape});
  Raster raster(40, 40);

  raster.draw(root);
  raster.draw(root);
  CHECK(shape.drawn == 1);

  shape.invalidate();
  raster.draw(root);
  CHECK(shape.drawn == 2);
}

TEST_CASE("a tree is walked again for another size or scale") {
  RootNode root;
  CountingNode shape;
  root.setChildren({&shape});
  Raster raster(40, 40);

  root.renderInto(raster.canvas(), {40, 40}, 1);
  root.renderInto(raster.canvas(), {40, 20}, 1);
  root.renderInto(raster.canvas(), {40, 20}, 2);
  root.renderInto(raster.canvas(), {40, 20}, 2);

  CHECK(shape.drawn == 3);
}

TEST_CASE("a tree played a few frames in a row is kept as an image") {
  RootNode root;
  PathNode path;
  path.setGeometry({.data = "M5 5 L35 5 L20 35 Z"});
  root.setChildren({&path});
  DrawProbe probe;

  for (int frame = 0; frame < 3; ++frame) root.renderInto(probe, {40, 40}, 1);
  CHECK(probe.paths == 3);
  CHECK(probe.images == 0);

  root.renderInto(probe, {40, 40}, 1);
  root.renderInto(probe, {40, 40}, 1);
  CHECK(probe.paths == 3);
  CHECK(probe.images == 2);
}

TEST_CASE("a tree kept as an image shows the same pixels") {
  RootNode root;
  PathNode path;
  path.setGeometry({.data = "M5 5 L35 5 L20 35 Z"});
  path.paint = filled(SK_ColorRED);
  root.setChildren({&path});
  DrawProbe probe;
  for (int frame = 0; frame < 4; ++frame) root.renderInto(probe, {40, 40}, 1);
  REQUIRE(probe.images == 1);
  Raster raster(60, 60);

  raster.canvas().translate(20, 20);
  root.renderInto(raster.canvas(), {40, 40}, 1);

  checkColor(raster.at(40, 30), SK_ColorRED);
  CHECK(raster.at(22, 55) == kClear);
  CHECK(raster.at(10, 10) == kClear);
}

TEST_CASE("a tree that changes goes back to being played") {
  RootNode root;
  PathNode path;
  path.setGeometry({.data = "M5 5 L35 5 L20 35 Z"});
  root.setChildren({&path});
  DrawProbe probe;
  for (int frame = 0; frame < 4; ++frame) root.renderInto(probe, {40, 40}, 1);
  REQUIRE(probe.images == 1);

  path.invalidate();
  root.renderInto(probe, {40, 40}, 1);

  CHECK(probe.images == 1);
  CHECK(probe.paths == 4);
}

TEST_CASE("a tree drawn through a scale or a fractional offset is always played") {
  RootNode root;
  PathNode path;
  path.setGeometry({.data = "M5 5 L35 5 L20 35 Z"});
  root.setChildren({&path});

  DrawProbe scaled;
  scaled.scale(1.5F, 1.5F);
  for (int frame = 0; frame < 6; ++frame) root.renderInto(scaled, {40, 40}, 1);
  CHECK(scaled.images == 0);

  DrawProbe shifted;
  shifted.translate(0.5F, 0);
  for (int frame = 0; frame < 6; ++frame) root.renderInto(shifted, {40, 40}, 1);
  CHECK(shifted.images == 0);
  CHECK(shifted.paths == 6);
}

TEST_CASE("an image waits for the canvas to show the tree pixel for pixel again") {
  RootNode root;
  PathNode path;
  path.setGeometry({.data = "M5 5 L35 5 L20 35 Z"});
  root.setChildren({&path});
  DrawProbe probe;
  for (int frame = 0; frame < 4; ++frame) root.renderInto(probe, {40, 40}, 1);
  REQUIRE(probe.images == 1);

  probe.save();
  probe.scale(1.1F, 1.1F);
  root.renderInto(probe, {40, 40}, 1);
  probe.restore();
  CHECK(probe.images == 1);

  root.renderInto(probe, {40, 40}, 1);
  CHECK(probe.images == 2);
}

TEST_CASE("a tree too large for an image, or one that draws nothing, is always played") {
  RootNode large;
  PathNode path;
  path.setGeometry({.data = "M5 5 L35 5 L20 35 Z"});
  large.setChildren({&path});
  DrawProbe probe;
  for (int frame = 0; frame < 6; ++frame) large.renderInto(probe, {600, 600}, 1);
  CHECK(probe.images == 0);
  CHECK(probe.paths == 6);

  RootNode empty;
  for (int frame = 0; frame < 6; ++frame) empty.renderInto(probe, {40, 40}, 1);
  CHECK(probe.images == 0);
}

TEST_CASE("a played tree draws where the canvas has moved to since it was walked") {
  RootNode root;
  RectNode rect;
  rect.setGeometry(box(0, 0, 10, 10));
  root.setChildren({&rect});
  Raster raster(40, 40);
  root.renderInto(raster.canvas(), {10, 10}, 1);

  raster.canvas().translate(20, 20);
  root.renderInto(raster.canvas(), {10, 10}, 1);

  checkColor(raster.at(5, 5), SK_ColorBLACK);
  checkColor(raster.at(25, 25), SK_ColorBLACK);
  CHECK(raster.at(15, 15) == kClear);
}

TEST_CASE("the first node with a name answers for it") {
  RootNode root;
  RectNode first;
  first.name = "tile";
  first.hidden = true;
  first.setGeometry(box(0, 0, 10, 10));
  RectNode second;
  second.name = "tile";
  second.hidden = true;
  second.setGeometry(box(0, 0, 40, 40));
  root.setChildren({&first, &second});

  const Definitions definitions(root);

  CHECK(definitions.find("tile") == &first);
  CHECK(definitions.find("") == nullptr);
  CHECK(definitions.findAs<ClipPathNode>("tile") == nullptr);
}

} // namespace rnwebos::svg
