#include "ImageContent.h"

#include "../render/RasterScene.test.h"

#include "include/core/SkBitmap.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using testing::nearColor;

constexpr PixelRect kBox{.x = 20, .y = 10, .width = 100, .height = 50};

// `width` x `height` pixels, the left half red and the right half blue.
sk_sp<SkImage> halves(int width, int height) {
  SkBitmap pixels;
  pixels.allocPixels(SkImageInfo::Make(width, height, kRGBA_8888_SkColorType, kOpaque_SkAlphaType));
  pixels.eraseColor(SK_ColorRED);
  pixels.erase(SK_ColorBLUE, SkIRect::MakeLTRB(width / 2, 0, width, height));
  pixels.setImmutable();
  return pixels.asImage();
}

struct Painted {
  explicit Painted(const ImageContent &content, float alpha = 1) {
    auto canvas = frame.canvas();
    canvas.clear(SK_ColorBLACK);
    content.draw(canvas, alpha);
  }

  testing::RasterScene frame{PixelSize{140, 70}};
};

ImageContent contentOf(ImageFit fit, sk_sp<SkImage> image, PixelSize size) {
  ImageContent content;
  content.setFit(fit);
  content.layout(kBox, 1);
  content.setImage(std::move(image), size);
  return content;
}

} // namespace

TEST_CASE("an image content without an image paints nothing") {
  ImageContent content;
  content.layout(kBox, 1);

  const Painted painted(content);

  CHECK_FALSE(content.hasImage());
  CHECK(painted.frame.pixel(70, 35) == SK_ColorBLACK);
}

TEST_CASE("stretch fills the box with the whole image and nothing outside it") {
  const Painted painted(contentOf(ImageFit::Stretch, halves(20, 20), {20, 20}));

  CHECK(nearColor(painted.frame.pixel(25, 15), SK_ColorRED));
  CHECK(nearColor(painted.frame.pixel(115, 55), SK_ColorBLUE));
  CHECK(painted.frame.pixel(19, 30) == SK_ColorBLACK);
  CHECK(painted.frame.pixel(121, 30) == SK_ColorBLACK);
  CHECK(painted.frame.pixel(70, 61) == SK_ColorBLACK);
}

TEST_CASE("contain leaves the box's sides bare and cover cuts the image's") {
  const Painted contained(contentOf(ImageFit::Contain, halves(20, 20), {20, 20}));
  const Painted covering(contentOf(ImageFit::Cover, halves(400, 50), {400, 50}));

  CHECK(contained.frame.pixel(30, 35) == SK_ColorBLACK);
  CHECK(nearColor(contained.frame.pixel(50, 35), SK_ColorRED));
  CHECK(nearColor(contained.frame.pixel(90, 35), SK_ColorBLUE));
  CHECK(contained.frame.pixel(110, 35) == SK_ColorBLACK);
  CHECK(nearColor(covering.frame.pixel(22, 35), SK_ColorRED));
  CHECK(nearColor(covering.frame.pixel(68, 35), SK_ColorRED));
  CHECK(nearColor(covering.frame.pixel(72, 35), SK_ColorBLUE));
  CHECK(covering.frame.pixel(125, 35) == SK_ColorBLACK);
}

TEST_CASE("an image decoded smaller than its file is placed as the file's size asks") {
  const Painted painted(contentOf(ImageFit::Cover, halves(100, 13), {400, 50}));

  CHECK(nearColor(painted.frame.pixel(22, 35), SK_ColorRED));
  CHECK(nearColor(painted.frame.pixel(66, 35), SK_ColorRED));
  CHECK(nearColor(painted.frame.pixel(74, 35), SK_ColorBLUE));
}

TEST_CASE("repeat tiles the image from the box's corner") {
  const Painted painted(contentOf(ImageFit::Repeat, halves(20, 20), {20, 20}));

  CHECK(nearColor(painted.frame.pixel(22, 12), SK_ColorRED));
  CHECK(nearColor(painted.frame.pixel(35, 12), SK_ColorBLUE));
  CHECK(nearColor(painted.frame.pixel(45, 52), SK_ColorRED));
  CHECK(nearColor(painted.frame.pixel(115, 35), SK_ColorBLUE));
  CHECK(painted.frame.pixel(121, 35) == SK_ColorBLACK);
}

TEST_CASE("a tint paints every pixel of the image in one color, and an opacity fades it") {
  auto content = contentOf(ImageFit::Stretch, halves(20, 20), {20, 20});
  content.setTint(SkColors::kGreen);

  const Painted tinted(content);
  const Painted faded(content, 0.5f);
  content.setTint(std::nullopt);
  const Painted plain(content);

  CHECK(nearColor(tinted.frame.pixel(30, 30), SK_ColorGREEN));
  CHECK(nearColor(tinted.frame.pixel(110, 30), SK_ColorGREEN));
  CHECK(nearColor(faded.frame.pixel(30, 30), SkColorSetRGB(0, 128, 0)));
  CHECK(nearColor(plain.frame.pixel(30, 30), SK_ColorRED));
}

TEST_CASE("clearing an image content empties it") {
  auto content = contentOf(ImageFit::Stretch, halves(20, 20), {20, 20});
  CHECK(content.hasImage());
  CHECK(content.size() == PixelSize{20, 20});

  content.clear();

  CHECK_FALSE(content.hasImage());
  CHECK(Painted(content).frame.pixel(70, 35) == SK_ColorBLACK);
}

} // namespace rnwebos
