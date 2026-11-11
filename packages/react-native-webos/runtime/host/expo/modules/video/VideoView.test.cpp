#include "VideoView.h"

#include "VideoPlayerHarness.test.h"

#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::awaitEvents;
using testing::createVideoPlayer;
using testing::ExpoTestRuntime;

namespace {

constexpr SkColor kBackdrop = SK_ColorRED;

/// What a page with a background is: opaque pixels everywhere, under the views.
class Backdrop final : public SceneLayer {
 public:
  void draw(SkCanvas &canvas) override {
    canvas.drawColor(kBackdrop);
  }
};

facebook::react::LayoutMetrics box(float x, float y, float width, float height) {
  facebook::react::LayoutMetrics metrics;
  metrics.frame = {.origin = {.x = x, .y = y}, .size = {.width = width, .height = height}};
  metrics.pointScaleFactor = 1;
  return metrics;
}

/// A scene with a backdrop, a root view over the whole of it and a VideoView in the root.
struct Stage {
  ExpoTestRuntime expo;
  Scene scene{PixelSize{960, 540}};
  Backdrop backdrop;
  ComponentView root{scene, 1};
  VideoView view{scene, 2};
  sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(960, 540));

  Stage() {
    scene.addLayer(backdrop, Scene::kSurfaceOrder - 1);
    root.mountAsRoot(Scene::kSurfaceOrder);
    root.updateLayout(box(0, 0, 960, 540));
    view.updateLayout(box(100, 100, 200, 200));
    root.mountChild(view, 0);
  }

  ~Stage() {
    root.unmountChild(view);
    scene.removeLayer(backdrop);
  }

  /// Shows a player of `source` in the view, once the source is ready.
  void show(const std::string &source) {
    const auto native = createVideoPlayer(expo, source);
    REQUIRE(awaitEvents(expo, "statusChange"));
    view.setPlayer(static_cast<double>(native->sharedObjectId()));
  }

  SkColor pixel(int x, int y) {
    scene.drawInto(*surface->getCanvas());
    SkPixmap pixels;
    REQUIRE(surface->peekPixels(&pixels));
    return pixels.getColor(x, y);
  }

  /// "backdrop" where the page shows, "hole" where the app's pixels are
  /// transparent and no picture is under them, "video" where the picture shows.
  std::string at(int x, int y) {
    const auto color = pixel(x, y);
    if (color == kBackdrop) return "backdrop";
    return SkColorGetA(color) == 0 ? "hole" : "video";
  }
};

} // namespace

TEST_CASE("a VideoView without a player leaves the page as it is") {
  Stage stage;
  CHECK(stage.at(200, 200) == "backdrop");
  stage.view.setContentFit("cover");
  stage.view.setPlayer(nullptr);
  CHECK(stage.at(200, 200) == "backdrop");
}

TEST_CASE("a VideoView clears its box and the picture shows in it, letterboxed by contain") {
  Stage stage;
  stage.show("'https://example.com/a.mp4#sim:load=0&size=800x400'");
  CHECK(stage.at(200, 200) == "video");
  CHECK(stage.at(200, 110) == "hole");
  CHECK(stage.at(200, 290) == "hole");
  CHECK(stage.at(101, 200) == "video");
  CHECK(stage.at(299, 200) == "video");
  CHECK(stage.at(99, 200) == "backdrop");
  CHECK(stage.at(300, 200) == "backdrop");
  CHECK(stage.at(200, 99) == "backdrop");
  CHECK(stage.at(200, 300) == "backdrop");
}

TEST_CASE("cover and fill put the picture over the whole box") {
  Stage stage;
  stage.show("'https://example.com/a.mp4#sim:load=0&size=800x400'");
  stage.view.setContentFit("cover");
  CHECK(stage.at(200, 110) == "video");
  CHECK(stage.at(200, 290) == "video");
  CHECK(stage.at(200, 300) == "backdrop");
  stage.view.setContentFit("fill");
  CHECK(stage.at(200, 110) == "video");
  stage.view.setContentFit("contain");
  CHECK(stage.at(200, 110) == "hole");
}

TEST_CASE("the hole and the picture follow the view's layout, and an ancestor that moves without telling it") {
  Stage stage;
  stage.show("'https://example.com/a.mp4#sim:load=0&size=400x400'");
  CHECK(stage.at(200, 200) == "video");
  stage.view.updateLayout(box(500, 200, 100, 100));
  CHECK(stage.at(200, 200) == "backdrop");
  CHECK(stage.at(550, 250) == "video");
  CHECK(stage.at(601, 250) == "backdrop");

  stage.root.updateLayout(box(200, 100, 960, 540));
  CHECK(stage.at(550, 250) == "backdrop");
  CHECK(stage.at(750, 350) == "video");
  CHECK(stage.at(701, 301) == "video");
  CHECK(stage.at(699, 350) == "backdrop");
}

TEST_CASE("fullscreen clears the whole window for the picture, and leaving it brings the page back") {
  Stage stage;
  stage.show("'https://example.com/a.mp4#sim:load=0&size=960x540'");
  stage.view.enterFullscreen();
  stage.view.enterFullscreen();
  CHECK(stage.at(5, 5) == "video");
  CHECK(stage.at(950, 530) == "video");
  stage.view.exitFullscreen();
  stage.view.exitFullscreen();
  CHECK(stage.at(5, 5) == "backdrop");
  CHECK(stage.at(200, 200) == "video");
  CHECK_THROWS_WITH(stage.view.startPictureInPicture(), "Picture in picture is not supported on webOS");
  stage.view.stopPictureInPicture();
}

TEST_CASE("the page closes over the video when the view leaves, loses its player, or the player is released") {
  Stage stage;
  stage.show("'https://example.com/a.mp4#sim:load=0&size=400x400'");
  CHECK(stage.at(200, 200) == "video");
  stage.root.unmountChild(stage.view);
  CHECK(stage.at(200, 200) == "backdrop");
  stage.root.mountChild(stage.view, 0);
  CHECK(stage.at(200, 200) == "video");

  stage.view.setPlayer(nullptr);
  CHECK(stage.at(200, 200) == "backdrop");
  stage.show("'https://example.com/b.mp4#sim:load=0&size=400x400'");
  CHECK(stage.at(200, 200) == "video");
  stage.expo.eval("player.release();");
  REQUIRE(stage.expo.pumpUntil([&stage] { return stage.at(200, 200) == "backdrop"; }));
}

TEST_CASE("the last view to show a player takes its picture, and gives it up without taking it from the next") {
  Stage stage;
  stage.show("'https://example.com/a.mp4#sim:load=0&size=400x400'");
  const auto id = stage.expo.eval("player.__expo_shared_object_id__").getNumber();
  VideoView second(stage.scene, 3);
  second.updateLayout(box(600, 300, 100, 100));
  stage.root.mountChild(second, 1);
  second.setPlayer(id);
  CHECK(stage.at(650, 350) == "video");
  CHECK(stage.at(200, 200) == "hole");
  stage.view.setPlayer(nullptr);
  CHECK(stage.at(650, 350) == "video");
  CHECK(stage.at(200, 200) == "backdrop");
  stage.root.unmountChild(second);
  CHECK(stage.at(650, 350) == "backdrop");
}

} // namespace rnwebos::expo
