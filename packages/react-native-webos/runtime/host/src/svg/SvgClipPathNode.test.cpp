#include "SvgClipPathNode.h"

#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;
constexpr int kEvenOdd = 0;
constexpr int kNonZero = 1;

} // namespace

TEST_CASE("a clip path keeps what is inside its children's outlines") {
  RootNode root;
  ClipPathNode clip;
  clip.name = "window";
  CircleNode hole;
  hole.setGeometry({.cx = {.value = 20}, .cy = {.value = 20}, .r = {.value = 10}});
  clip.setChildren({&hole});
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  rect.paint = filled(SK_ColorRED);
  rect.clipPath = "window";
  root.setChildren({&clip, &rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 20), SK_ColorRED);
  CHECK(raster.at(3, 3) == kClear);
  CHECK(raster.at(20, 5) == kClear);
}

TEST_CASE("a clip path follows its own matrix and its children's") {
  RootNode root;
  ClipPathNode clip;
  clip.name = "window";
  clip.matrix = SkMatrix::Translate(20, 0);
  RectNode opening;
  opening.matrix = SkMatrix::Translate(0, 20);
  opening.setGeometry(box(0, 0, 10, 10));
  clip.setChildren({&opening});
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  rect.clipPath = "window";
  root.setChildren({&clip, &rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(25, 25), SK_ColorBLACK);
  CHECK(raster.at(5, 5) == kClear);
  CHECK(raster.at(25, 5) == kClear);
}

TEST_CASE("the clip rule makes overlapping outlines a hole or a union") {
  RootNode root;
  ClipPathNode clip;
  clip.name = "pair";
  RectNode left;
  left.setGeometry(box(0, 0, 25, 40));
  RectNode right;
  right.setGeometry(box(15, 0, 25, 40));
  clip.setChildren({&left, &right});
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  rect.clipPath = "pair";
  root.setChildren({&clip, &rect});

  rect.clipRule = kEvenOdd;
  Raster evenOdd(40, 40);
  evenOdd.draw(root);
  checkColor(evenOdd.at(5, 20), SK_ColorBLACK);
  CHECK(evenOdd.at(20, 20) == kClear);

  rect.clipRule = kNonZero;
  Raster nonZero(40, 40);
  nonZero.draw(root);
  checkColor(nonZero.at(20, 20), SK_ColorBLACK);
}

TEST_CASE("a clip path drawn nowhere else draws nothing itself") {
  RootNode root;
  ClipPathNode clip;
  RectNode opening;
  opening.setGeometry(box(0, 0, 40, 40));
  clip.setChildren({&opening});
  root.setChildren({&clip});
  Raster raster(40, 40);

  raster.draw(root);

  CHECK(raster.at(20, 20) == kClear);
}

} // namespace rnwebos::svg
