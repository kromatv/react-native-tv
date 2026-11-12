#include "Scene.h"

#include "RasterTarget.test.h"

#include <doctest/doctest.h>

#include <functional>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

using testing::RasterTarget;

constexpr PixelSize kSize{.width = 8, .height = 8};

class RecordingLayer final : public SceneLayer {
 public:
  RecordingLayer(std::string name, std::vector<std::string> &drawn, SkColor color = SK_ColorTRANSPARENT)
      : name_(std::move(name)), drawn_(drawn), color_(color) {}

  void draw(SkCanvas &canvas) override {
    drawn_.push_back(name_);
    if (color_ != SK_ColorTRANSPARENT) canvas.drawColor(color_);
    if (whileDrawing) whileDrawing();
  }

  std::function<void()> whileDrawing;

 private:
  const std::string name_;
  std::vector<std::string> &drawn_;
  const SkColor color_;
};

} // namespace

TEST_CASE("Scene draws its layers lowest order first, and layers of one order in the order they were added") {
  Scene scene(kSize);
  RasterTarget target(kSize);
  std::vector<std::string> drawn;
  RecordingLayer error("error", drawn);
  RecordingLayer surface("surface", drawn, SK_ColorRED);
  RecordingLayer dialog("dialog", drawn, SK_ColorBLUE);
  RecordingLayer logBox("logbox", drawn);
  RecordingLayer modal("modal", drawn, SK_ColorGREEN);

  scene.addLayer(error, Scene::kErrorOrder);
  scene.addLayer(surface, Scene::kSurfaceOrder);
  scene.addLayer(dialog, Scene::kDialogOrder);
  scene.addLayer(logBox, Scene::kSurfaceOrder);
  scene.addLayer(modal, Scene::kModalOrder);
  scene.drawInto(target.canvas());

  CHECK(drawn == std::vector<std::string>{"surface", "logbox", "modal", "dialog", "error"});
  CHECK(target.at(4, 4) == SK_ColorBLUE);
}

TEST_CASE("Scene stops drawing a layer once it is removed") {
  Scene scene(kSize);
  RasterTarget target(kSize);
  std::vector<std::string> drawn;
  RecordingLayer surface("surface", drawn, SK_ColorRED);
  RecordingLayer banner("banner", drawn, SK_ColorBLUE);
  scene.addLayer(surface, Scene::kSurfaceOrder);
  scene.addLayer(banner, Scene::kBannerOrder);

  scene.removeLayer(banner);
  scene.drawInto(target.canvas());

  CHECK(drawn == std::vector<std::string>{"surface"});
  CHECK(target.at(4, 4) == SK_ColorRED);
}

TEST_CASE("Scene draws the layers it had when the frame started, whatever a layer adds or removes while drawing") {
  Scene scene(kSize);
  RasterTarget target(kSize);
  std::vector<std::string> drawn;
  RecordingLayer surface("surface", drawn);
  RecordingLayer modal("modal", drawn);
  RecordingLayer late("late", drawn);
  scene.addLayer(surface, Scene::kSurfaceOrder);
  scene.addLayer(modal, Scene::kModalOrder);
  surface.whileDrawing = [&scene, &late] { scene.addLayer(late, Scene::kDialogOrder); };

  scene.drawInto(target.canvas());

  CHECK(drawn == std::vector<std::string>{"surface", "modal"});
  surface.whileDrawing = nullptr;
  drawn.clear();
  scene.drawInto(target.canvas());
  CHECK(drawn == std::vector<std::string>{"surface", "modal", "late"});
}

TEST_CASE("Scene clears to black, or to transparent when the video plane must show through") {
  Scene scene(kSize);
  RasterTarget target(kSize);

  scene.drawInto(target.canvas());
  CHECK(target.at(0, 0) == SK_ColorBLACK);

  scene.setTransparent(true);
  scene.drawInto(target.canvas());
  CHECK(target.at(0, 0) == SK_ColorTRANSPARENT);
}

TEST_CASE("Scene is invalid from the start and after every change, until it renders") {
  Scene scene(kSize);
  std::vector<std::string> drawn;
  RecordingLayer layer("layer", drawn);
  const auto settle = [&scene] {
    scene.render();
    return !scene.isInvalid();
  };

  CHECK(scene.isInvalid());
  CHECK(settle());

  scene.addLayer(layer, Scene::kSurfaceOrder);
  CHECK(scene.isInvalid());
  CHECK(settle());

  scene.invalidate();
  CHECK(scene.isInvalid());
  CHECK(settle());

  scene.setTransparent(true);
  CHECK(scene.isInvalid());
  CHECK(settle());
  scene.setTransparent(true);
  CHECK_FALSE(scene.isInvalid());

  scene.removeLayer(layer);
  CHECK(scene.isInvalid());
  CHECK(settle());
  scene.removeLayer(layer);
  CHECK_FALSE(scene.isInvalid());
}

TEST_CASE("A scene without a window has its size, presents nothing and draws only when asked") {
  Scene scene(kSize);
  std::vector<std::string> drawn;
  RecordingLayer layer("layer", drawn);
  scene.addLayer(layer, Scene::kSurfaceOrder);

  CHECK(scene.window() == nullptr);
  CHECK(scene.size() == kSize);
  CHECK_FALSE(scene.render());
  CHECK(drawn.empty());
}

} // namespace rnwebos
