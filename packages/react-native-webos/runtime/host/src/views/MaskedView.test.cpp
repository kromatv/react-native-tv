#include "MaskedView.h"

#include "../render/RasterScene.test.h"
#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using testing::filled;
using testing::frameOf;
using testing::nearColor;
using testing::place;

struct Stage {
  Stage() {
    place(masked, std::make_shared<facebook::react::ViewProps>(), frameOf(20, 10, 100, 60));
    place(content, filled(255, 0, 0), frameOf(-20, -10, 200, 100));
    root->mountChild(masked, 0);
  }

  testing::RasterScene frame{PixelSize{200, 100}};
  std::unique_ptr<ComponentView> root{testing::rootOf(frame.scene())};
  MaskedView masked{frame.scene(), 2};
  ComponentView mask{frame.scene(), 3};
  ComponentView content{frame.scene(), 4};
};

} // namespace

TEST_CASE("a masked view shows its content only where the mask is opaque, inside its own box") {
  Stage stage;
  place(stage.mask, filled(0, 0, 0), frameOf(0, 0, 50, 60));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);

  stage.frame.draw();

  CHECK(nearColor(stage.frame.pixel(30, 30), SK_ColorRED));
  CHECK(nearColor(stage.frame.pixel(68, 68), SK_ColorRED));
  CHECK(stage.frame.pixel(72, 30) == SK_ColorBLACK);
  CHECK(stage.frame.pixel(10, 30) == SK_ColorBLACK);
  CHECK(stage.frame.pixel(30, 75) == SK_ColorBLACK);
}

TEST_CASE("a translucent mask lets its share of the content through, whatever the mask's color") {
  Stage stage;
  place(stage.mask, filled(0, 255, 0, 128), frameOf(0, 0, 100, 60));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);

  stage.frame.draw();

  CHECK(nearColor(stage.frame.pixel(60, 40), SkColorSetRGB(128, 0, 0)));
}

TEST_CASE("the mask alone draws nothing, and neither does a masked view without children") {
  Stage stage;
  stage.frame.draw();
  CHECK(stage.frame.pixel(60, 40) == SK_ColorBLACK);

  place(stage.mask, filled(255, 255, 255), frameOf(0, 0, 100, 60));
  stage.masked.mountChild(stage.mask, 0);
  stage.frame.draw();

  CHECK(stage.frame.pixel(60, 40) == SK_ColorBLACK);
}

} // namespace rnwebos
