#include "LoadingBanner.h"

#include "render/RasterTarget.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

constexpr PixelSize kScreen{.width = 1280, .height = 720};
constexpr std::uint32_t kWhite = 0xffffffff;
constexpr std::uint32_t kGrey = 0xff404040;
constexpr int kStripHeight = 40;

int pixelsUnlike(const testing::RasterTarget &target, SkColor color, int rows) {
  int count = 0;
  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < kScreen.width; ++x) {
      if (target.at(x, y) != color) ++count;
    }
  }
  return count;
}

} // namespace

TEST_CASE("LoadingBanner draws its message in a strip across the top of the screen, in the colors given") {
  Scene scene(kScreen);
  testing::RasterTarget target(kScreen);
  LoadingBanner banner(scene);
  scene.render();

  banner.show("Loading from 10.0.0.2:8081", kWhite, kGrey);

  CHECK(banner.isShown());
  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());
  CHECK(target.at(2, 2) == SkColorSetRGB(0x40, 0x40, 0x40));
  CHECK(target.at(kScreen.width - 1, kStripHeight - 1) == SkColorSetRGB(0x40, 0x40, 0x40));
  CHECK(target.at(2, kStripHeight) == SK_ColorBLACK);
  CHECK(pixelsUnlike(target, SkColorSetRGB(0x40, 0x40, 0x40), kStripHeight) > 300);
  CHECK(target.at(2, 20) == SkColorSetRGB(0x40, 0x40, 0x40));
}

TEST_CASE("LoadingBanner replaces its message, blends a translucent strip and leaves the scene when hidden") {
  Scene scene(kScreen);
  testing::RasterTarget target(kScreen);
  LoadingBanner banner(scene);

  banner.show("Refreshing", kWhite, kGrey);
  banner.show("Paused in debugger", kWhite, 0x80ff0000);
  scene.drawInto(target.canvas());
  CHECK(SkColorGetR(target.at(2, 2)) == doctest::Approx(128).epsilon(0.02));
  CHECK(SkColorGetG(target.at(2, 2)) == 0);

  scene.render();
  banner.hide();
  CHECK_FALSE(banner.isShown());
  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());
  CHECK(target.at(2, 2) == SK_ColorBLACK);
  banner.hide();
}

TEST_CASE("LoadingBanner clips a message longer than the screen to its strip") {
  Scene scene(kScreen);
  testing::RasterTarget target(kScreen);
  LoadingBanner banner(scene);

  banner.show(std::string(400, 'W'), kWhite, kGrey);
  scene.drawInto(target.canvas());

  CHECK(pixelsUnlike(target, SK_ColorBLACK, kScreen.height) - pixelsUnlike(target, SK_ColorBLACK, kStripHeight) == 0);
}

} // namespace rnwebos
