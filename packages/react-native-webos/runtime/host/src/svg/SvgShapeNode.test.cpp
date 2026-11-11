#include "SvgShapeNode.h"

#include "SvgRaster.test.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;
constexpr int kEvenOdd = 0;
constexpr int kNonZero = 1;
constexpr auto kFillRule = static_cast<std::size_t>(PaintAttribute::FillRule);

} // namespace

TEST_CASE("a shape fills black unless told otherwise") {
  RootNode root;
  RectNode rect;
  rect.setGeometry(box(10, 10, 20, 20));
  root.setChildren({&rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 20), SK_ColorBLACK);
  CHECK(raster.at(5, 5) == kClear);
}

TEST_CASE("a stroke is centered on the path, as wide as strokeWidth") {
  RootNode root;
  LineNode line;
  line.setGeometry({.x1 = {.value = 0}, .y1 = {.value = 20}, .x2 = {.value = 40}, .y2 = {.value = 20}});
  line.paint = stroked(SK_ColorWHITE, 10);
  root.setChildren({&line});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 16), SK_ColorWHITE);
  checkColor(raster.at(20, 24), SK_ColorWHITE);
  CHECK(raster.at(20, 13) == kClear);
  CHECK(raster.at(20, 26) == kClear);
}

TEST_CASE("a stroke width given as a percentage resolves against the viewport's diagonal") {
  RootNode root;
  LineNode line;
  line.setGeometry({.x1 = {.value = 0}, .y1 = {.value = 20}, .x2 = {.value = 40}, .y2 = {.value = 20}});
  line.paint = stroked(SK_ColorWHITE, 0);
  line.paint.strokeWidth = {.value = 50, .percent = true};
  root.setChildren({&line});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 11), SK_ColorWHITE);
  CHECK(raster.at(20, 8) == kClear);
}

TEST_CASE("a square cap extends the line by half its width and a butt cap does not") {
  RootNode root;
  LineNode butt;
  butt.setGeometry({.x1 = {.value = 10}, .y1 = {.value = 10}, .x2 = {.value = 30}, .y2 = {.value = 10}});
  butt.paint = stroked(SK_ColorWHITE, 8);
  LineNode square;
  square.setGeometry({.x1 = {.value = 10}, .y1 = {.value = 30}, .x2 = {.value = 30}, .y2 = {.value = 30}});
  square.paint = stroked(SK_ColorWHITE, 8);
  square.paint.strokeLinecap = 2;
  root.setChildren({&butt, &square});
  Raster raster(40, 40);

  raster.draw(root);

  CHECK(raster.at(8, 10) == kClear);
  checkColor(raster.at(8, 30), SK_ColorWHITE);
  CHECK(raster.at(4, 30) == kClear);
}

TEST_CASE("dashes leave gaps, and the dash offset shifts them") {
  RootNode root;
  LineNode line;
  line.setGeometry({.x1 = {.value = 0}, .y1 = {.value = 10}, .x2 = {.value = 40}, .y2 = {.value = 10}});
  line.paint = stroked(SK_ColorWHITE, 4);
  line.paint.strokeDasharray = {{.value = 10}, {.value = 10}};
  root.setChildren({&line});
  Raster raster(40, 20);

  raster.draw(root);
  checkColor(raster.at(5, 10), SK_ColorWHITE);
  CHECK(raster.at(15, 10) == kClear);
  checkColor(raster.at(25, 10), SK_ColorWHITE);

  line.paint.strokeDashoffset = 10;
  line.invalidate();
  Raster shifted(40, 20);
  shifted.draw(root);
  CHECK(shifted.at(5, 10) == kClear);
  checkColor(shifted.at(15, 10), SK_ColorWHITE);
}

TEST_CASE("an odd dash list repeats once") {
  RootNode root;
  LineNode line;
  line.setGeometry({.x1 = {.value = 0}, .y1 = {.value = 10}, .x2 = {.value = 40}, .y2 = {.value = 10}});
  line.paint = stroked(SK_ColorWHITE, 4);
  line.paint.strokeDasharray = {{.value = 10}};
  root.setChildren({&line});
  Raster raster(40, 20);

  raster.draw(root);

  checkColor(raster.at(5, 10), SK_ColorWHITE);
  CHECK(raster.at(15, 10) == kClear);
}

TEST_CASE("the fill rule decides whether a path's inner loop is a hole") {
  const std::string rings = "M0 0 H40 V40 H0 Z M10 10 H30 V30 H10 Z";
  RootNode root;
  PathNode path;
  path.setGeometry({.data = rings});
  path.paint = filled(SK_ColorRED);
  path.paint.listed.set(kFillRule);
  root.setChildren({&path});

  path.paint.fillRule = kEvenOdd;
  Raster evenOdd(40, 40);
  evenOdd.draw(root);
  CHECK(evenOdd.at(20, 20) == kClear);
  checkColor(evenOdd.at(5, 5), SK_ColorRED);

  path.paint.fillRule = kNonZero;
  path.invalidate();
  Raster nonZero(40, 40);
  nonZero.draw(root);
  checkColor(nonZero.at(20, 20), SK_ColorRED);
}

TEST_CASE("fill and stroke opacity fade their own paint") {
  RootNode root;
  RectNode rect;
  rect.setGeometry(box(10, 10, 20, 20));
  rect.paint = filled(SK_ColorRED);
  rect.paint.fillOpacity = 0.5F;
  root.setChildren({&rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 20), SkColorSetARGB(128, 255, 0, 0));
}

} // namespace rnwebos::svg
