#include "SvgRootNode.h"

#include "SvgClipPathNode.h"
#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;

} // namespace

TEST_CASE("a tree counts the paints it lays over each other") {
  RootNode root;
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  root.setChildren({&rect});
  CHECK(root.paintLayers({40, 40}) == 1);

  rect.paint = stroked(SK_ColorWHITE, 2);
  rect.paint.fill = solidBrush(SK_ColorRED);
  rect.invalidate();
  CHECK(root.paintLayers({40, 40}) == 2);

  rect.hidden = true;
  CHECK(root.paintLayers({40, 40}) == 0);
}

TEST_CASE("the alpha a view hands a tree of one paint fades that paint") {
  RootNode root;
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  rect.paint = filled(SK_ColorRED);
  root.setChildren({&rect});
  Raster raster(40, 40);

  root.renderInto(raster.canvas(), {40, 40}, 1, 0.5F);

  checkColor(raster.at(20, 20), SkColorSetARGB(128, 255, 0, 0));
}

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
