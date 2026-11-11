#include "ScreenCapture.h"

#include <doctest/doctest.h>

#include "include/core/SkPaint.h"

#include <array>
#include <cstdint>

namespace rnwebos {

namespace {

class PaintedLayer final : public SceneLayer {
 public:
  void draw(SkCanvas &canvas) override {
    SkPaint paint;
    paint.setColor(SkColorSetARGB(0xff, 0x11, 0x22, 0x33));
    canvas.drawRect(SkRect::MakeXYWH(1, 0, 2, 1), paint);
    paint.setColor(SkColorSetARGB(0x80, 0xff, 0x00, 0x00));
    paint.setBlendMode(SkBlendMode::kSrc);
    canvas.drawRect(SkRect::MakeXYWH(0, 2, 1, 1), paint);
  }
};

std::array<std::uint8_t, 4> bytesAt(const ScreenCapture &capture, int x, int y) {
  const auto *pixel = capture.pixels.data() + y * capture.stride + x * 4;
  return {pixel[0], pixel[1], pixel[2], pixel[3]};
}

} // namespace

TEST_CASE("captureScene gives a scene without a window as premultiplied BGRA rows, top row first") {
  Scene scene(PixelSize{.width = 4, .height = 3});
  PaintedLayer layer;
  scene.addLayer(layer, Scene::kSurfaceOrder);

  const auto capture = captureScene(scene);

  REQUIRE(capture.has_value());
  CHECK(capture->width == 4);
  CHECK(capture->height == 3);
  CHECK(capture->stride == 16);
  CHECK(capture->pixels.size() == 48);
  CHECK(bytesAt(*capture, 0, 0) == std::array<std::uint8_t, 4>{0x00, 0x00, 0x00, 0xff});
  CHECK(bytesAt(*capture, 1, 0) == std::array<std::uint8_t, 4>{0x33, 0x22, 0x11, 0xff});
  CHECK(bytesAt(*capture, 2, 0) == std::array<std::uint8_t, 4>{0x33, 0x22, 0x11, 0xff});
  CHECK(bytesAt(*capture, 3, 0) == std::array<std::uint8_t, 4>{0x00, 0x00, 0x00, 0xff});
  CHECK(bytesAt(*capture, 0, 2) == std::array<std::uint8_t, 4>{0x00, 0x00, 0x80, 0x80});
}

TEST_CASE("captureScene keeps the transparency of a scene that shows the video plane through") {
  Scene scene(PixelSize{.width = 2, .height = 2});
  scene.setTransparent(true);

  const auto capture = captureScene(scene);

  REQUIRE(capture.has_value());
  CHECK(bytesAt(*capture, 1, 1) == std::array<std::uint8_t, 4>{0, 0, 0, 0});
}

TEST_CASE("captureScene gives nothing for a scene with no pixels") {
  Scene scene(PixelSize{.width = 0, .height = 0});

  CHECK_FALSE(captureScene(scene).has_value());
}

} // namespace rnwebos
