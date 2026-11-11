#include "SvgGroupNode.h"

#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;

} // namespace

TEST_CASE("a group's opacity fades its children as one picture") {
  RootNode root;
  GroupNode group;
  group.opacity = 0.5F;
  RectNode under;
  under.setGeometry(box(0, 0, 30, 30));
  under.paint = filled(SK_ColorRED);
  RectNode over;
  over.setGeometry(box(10, 10, 30, 30));
  over.paint = filled(SK_ColorBLUE);
  group.setChildren({&under, &over});
  root.setChildren({&group});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(5, 5), SkColorSetARGB(128, 255, 0, 0));
  checkColor(raster.at(20, 20), SkColorSetARGB(128, 0, 0, 255));
}

TEST_CASE("an opacity on one paint needs no layer, and one on several takes one") {
  RootNode root;
  GroupNode group;
  group.opacity = 0.5F;
  PathNode first;
  first.setGeometry({.data = "M0 0 H40 V40 Z"});
  first.paint = filled(SK_ColorRED);
  first.opacity = 0.5F;
  group.setChildren({&first});
  root.setChildren({&group});

  DrawProbe single;
  root.renderInto(single, {40, 40}, 1);
  CHECK(single.layers == 0);
  Raster raster(40, 40);
  raster.draw(root);
  checkColor(raster.at(30, 10), SkColorSetARGB(64, 255, 0, 0));

  PathNode second;
  second.setGeometry({.data = "M0 0 H40 V40 Z"});
  group.setChildren({&first, &second});
  DrawProbe both;
  root.renderInto(both, {40, 40}, 1);
  CHECK(both.layers == 1);
}

TEST_CASE("a hidden node and its subtree draw nothing") {
  RootNode root;
  GroupNode group;
  group.hidden = true;
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  group.setChildren({&rect});
  root.setChildren({&group});
  Raster raster(40, 40);

  raster.draw(root);

  CHECK(raster.at(20, 20) == kClear);
}

TEST_CASE("a node's matrix applies to its subtree, the parent's first") {
  RootNode root;
  GroupNode group;
  group.matrix = SkMatrix::Translate(20, 0);
  RectNode rect;
  rect.matrix = SkMatrix::Scale(2, 2);
  rect.setGeometry(box(0, 0, 5, 5));
  group.setChildren({&rect});
  root.setChildren({&group});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(25, 5), SK_ColorBLACK);
  CHECK(raster.at(15, 5) == kClear);
  CHECK(raster.at(32, 5) == kClear);
}

TEST_CASE("a child inherits the paint its parent lists") {
  RootNode root;
  GroupNode group;
  group.paint = filled(SK_ColorRED);
  RectNode rect;
  rect.setGeometry(box(0, 0, 40, 40));
  group.setChildren({&rect});
  root.setChildren({&group});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(20, 20), SK_ColorRED);
}

TEST_CASE("currentColor is the nearest color set above the shape") {
  RootNode root;
  root.color = SkColor4f::FromColor(SK_ColorRED);
  RectNode fromRoot;
  fromRoot.setGeometry(box(0, 0, 20, 40));
  fromRoot.paint.fill = {.kind = Brush::Kind::CurrentColor};
  GroupNode group;
  group.color = SkColor4f::FromColor(SK_ColorBLUE);
  RectNode fromGroup;
  fromGroup.setGeometry(box(20, 0, 20, 40));
  fromGroup.paint.fill = {.kind = Brush::Kind::CurrentColor};
  group.setChildren({&fromGroup});
  root.setChildren({&fromRoot, &group});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(10, 20), SK_ColorRED);
  checkColor(raster.at(30, 20), SK_ColorBLUE);
}

} // namespace rnwebos::svg
