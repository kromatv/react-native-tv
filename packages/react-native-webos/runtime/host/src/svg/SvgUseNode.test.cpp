#include "SvgUseNode.h"

#include "SvgRaster.test.h"
#include "SvgShapeNode.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr SkColor kClear = SK_ColorTRANSPARENT;

} // namespace

TEST_CASE("a use draws its target at x and y, with the paint the use hands down") {
  RootNode root;
  DefinitionsNode defs;
  RectNode shape;
  shape.name = "tile";
  shape.setGeometry(box(0, 0, 10, 10));
  defs.setChildren({&shape});
  UseNode use;
  use.href = "tile";
  use.x = {.value = 20};
  use.y = {.value = 10};
  use.paint = filled(SK_ColorRED);
  root.setChildren({&defs, &use});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(25, 15), SK_ColorRED);
  CHECK(raster.at(5, 5) == kClear);
}

TEST_CASE("a use fits a symbol's viewBox into its width and height") {
  RootNode root;
  SymbolNode symbol;
  symbol.name = "dot";
  symbol.viewBox = {.width = 10, .height = 10, .align = "xMidYMid"};
  RectNode shape;
  shape.setGeometry(box(0, 0, 10, 10));
  symbol.setChildren({&shape});
  UseNode use;
  use.href = "dot";
  use.x = {.value = 10};
  use.y = {.value = 10};
  use.width = {.value = 20};
  use.height = {.value = 20};
  root.setChildren({&symbol, &use});
  Raster raster(40, 40);

  raster.draw(root);

  checkColor(raster.at(12, 12), SK_ColorBLACK);
  checkColor(raster.at(28, 28), SK_ColorBLACK);
  CHECK(raster.at(5, 5) == kClear);
  CHECK(raster.at(33, 33) == kClear);
}

TEST_CASE("a use that names itself or nothing draws nothing") {
  RootNode root;
  UseNode loop;
  loop.name = "loop";
  loop.href = "loop";
  UseNode dangling;
  dangling.href = "missing";
  root.setChildren({&loop, &dangling});
  Raster raster(40, 40);

  raster.draw(root);

  CHECK(raster.at(20, 20) == kClear);
}

} // namespace rnwebos::svg
