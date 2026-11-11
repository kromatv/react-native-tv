#include "SvgMaskNode.h"

#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;

} // namespace

TEST_CASE("a luminance mask shows a node as bright as the mask is") {
  RootNode root;
  MaskNode mask;
  mask.name = "half";
  RectNode white;
  white.setGeometry(box(0, 0, 20, 40));
  white.paint = filled(SK_ColorWHITE);
  RectNode gray;
  gray.setGeometry(box(20, 0, 10, 40));
  gray.paint = filled(SkColorSetRGB(128, 128, 128));
  mask.setChildren({&white, &gray});
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  rect.paint = filled(SK_ColorRED);
  rect.mask = "half";
  root.setChildren({&mask, &rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(10, 20), SK_ColorRED);
  CHECK(SkColorGetA(raster.at(25, 20)) > 40);
  CHECK(SkColorGetA(raster.at(25, 20)) < 140);
  CHECK(raster.at(35, 20) == kClear);
}

TEST_CASE("an alpha mask shows a node as opaque as the mask is, whatever its color") {
  RootNode root;
  MaskNode mask;
  mask.name = "half";
  mask.alphaOnly = true;
  RectNode black;
  black.setGeometry(box(0, 0, 20, 40));
  mask.setChildren({&black});
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  rect.paint = filled(SK_ColorRED);
  rect.mask = "half";
  root.setChildren({&mask, &rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(10, 20), SK_ColorRED);
  CHECK(raster.at(30, 20) == kClear);
}

TEST_CASE("a mask's area is a share of the masked node's box, or user units") {
  RootNode root;
  MaskNode mask;
  mask.name = "left";
  mask.width = {.value = 50, .percent = true};
  RectNode white;
  white.setGeometry(box(0, 0, 40, 40));
  white.paint = filled(SK_ColorWHITE);
  mask.setChildren({&white});
  RectNode rect;
  rect.setGeometry(box(20, 0, 20, 40));
  rect.paint = filled(SK_ColorRED);
  rect.mask = "left";
  root.setChildren({&mask, &rect});

  Raster ofBox(40, 40);
  ofBox.draw(root);
  checkColor(ofBox.at(25, 20), SK_ColorRED);
  CHECK(ofBox.at(35, 20) == kClear);

  mask.areaInUserSpace = true;
  mask.x = {.value = 30};
  mask.width = {.value = 10};
  Raster inUserSpace(40, 40);
  inUserSpace.draw(root);
  CHECK(inUserSpace.at(25, 20) == kClear);
  checkColor(inUserSpace.at(35, 20), SK_ColorRED);
}

TEST_CASE("a mask's content can be laid out in the masked node's box") {
  RootNode root;
  MaskNode mask;
  mask.name = "top";
  mask.contentInBoundingBox = true;
  RectNode white;
  white.setGeometry(box(0, 0, 1, 0.5F));
  white.paint = filled(SK_ColorWHITE);
  mask.setChildren({&white});
  RectNode rect;
  rect.setGeometry(box(10, 10, 20, 20));
  rect.paint = filled(SK_ColorRED);
  rect.mask = "top";
  root.setChildren({&mask, &rect});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 15), SK_ColorRED);
  CHECK(raster.at(20, 25) == kClear);
}

} // namespace rnwebos::svg
