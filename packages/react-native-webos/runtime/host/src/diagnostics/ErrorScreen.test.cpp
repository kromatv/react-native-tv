#include "ErrorScreen.h"

#include "../render/RasterTarget.test.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rnwebos {

namespace {

constexpr PixelSize kScreen{.width = 640, .height = 360};
constexpr SkColor kRedScreen = SkColorSetRGB(0x9b, 0x1c, 0x1c);

int pixelsUnlike(const testing::RasterTarget &target, SkColor color) {
  int count = 0;
  for (int y = 0; y < kScreen.height; ++y) {
    for (int x = 0; x < kScreen.width; ++x) {
      if (target.at(x, y) != color) ++count;
    }
  }
  return count;
}

} // namespace

TEST_CASE("ErrorScreen shows a report whose text holds markup characters, over everything else") {
  Scene scene(kScreen);
  testing::RasterTarget target(kScreen);
  const std::vector<std::string> stack(30, "at <anonymous> (index.bundle:1:2)");
  ErrorScreen screen(scene);
  scene.drawInto(target.canvas());
  CHECK_FALSE(screen.isShown());
  CHECK(target.at(1, 1) == SK_ColorBLACK);
  scene.render();

  screen.show({.title = "TypeError", .message = "a < b && c > d", .stack = stack});

  CHECK(screen.isShown());
  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());
  CHECK(target.at(1, 1) == kRedScreen);
  CHECK(target.at(kScreen.width - 1, kScreen.height - 1) == kRedScreen);
  CHECK(pixelsUnlike(target, kRedScreen) > 2000);
}

TEST_CASE("ErrorScreen replaces the report it shows, and leaves the scene when destroyed") {
  Scene scene(kScreen);
  testing::RasterTarget first(kScreen);
  testing::RasterTarget second(kScreen);
  {
    ErrorScreen screen(scene);
    screen.show({.title = "Error", .message = "short", .stack = {}});
    scene.drawInto(first.canvas());

    screen.show({.title = "Error", .message = std::string(400, 'W'), .stack = {}});
    scene.drawInto(second.canvas());

    CHECK(pixelsUnlike(second, kRedScreen) > pixelsUnlike(first, kRedScreen));
  }

  scene.drawInto(first.canvas());
  CHECK(first.at(1, 1) == SK_ColorBLACK);
}

} // namespace rnwebos
