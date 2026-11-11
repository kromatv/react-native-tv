#include "ImageDecoder.h"

#include "../platform/MainLoop.h"
#include "SampleImages.test.h"

#include "../render/RasterScene.test.h"

#include "include/core/SkPixmap.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <thread>

namespace rnwebos {

namespace {

using testing::imageBytes;
using testing::nearColor;

ImageDecodeRequest requestFor(std::string_view base64, PixelSize box = {}, ImageFit fit = ImageFit::Cover) {
  return {.bytes = imageBytes(base64), .path = {}, .fit = fit, .box = box, .naturalScale = 1};
}

SkColor pixelOf(const sk_sp<SkImage> &image, int x, int y) {
  SkPixmap pixels;
  REQUIRE(image->peekPixels(&pixels));
  return pixels.getColor(x, y);
}

void pump(const bool &done) {
  const auto giveUp = MainLoop::Clock::now() + std::chrono::seconds(5);
  while (!done && MainLoop::Clock::now() < giveUp) {
    MainLoop::get().iterate();
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
}

} // namespace

TEST_CASE("a PNG, a JPEG, a WebP and a GIF decode to opaque RGBA pixels at their own size") {
  for (const auto sample : {testing::kPng, testing::kJpeg, testing::kWebp, testing::kGif}) {
    const auto decoded = decodeImageNow(requestFor(sample));

    CHECK(decoded.error.empty());
    REQUIRE(decoded.image != nullptr);
    CHECK(decoded.size == PixelSize{64, 48});
    CHECK(decoded.image->width() == 64);
    CHECK(decoded.image->height() == 48);
    CHECK(decoded.image->colorType() == kRGBA_8888_SkColorType);
    CHECK(nearColor(pixelOf(decoded.image, 8, 24), SK_ColorRED, 6));
    CHECK(nearColor(pixelOf(decoded.image, 56, 24), SK_ColorBLUE, 6));
  }
}

TEST_CASE("an image at least twice its box is decoded at the size it is drawn at, whatever the format") {
  for (const auto sample : {testing::kPng, testing::kJpeg, testing::kWebp, testing::kGif}) {
    const auto decoded = decodeImageNow(requestFor(sample, {16, 12}));

    REQUIRE(decoded.image != nullptr);
    CHECK(decoded.size == PixelSize{64, 48});
    CHECK(decoded.image->width() == 16);
    CHECK(decoded.image->height() == 12);
    CHECK(nearColor(pixelOf(decoded.image, 2, 6), SK_ColorRED, 6));
    CHECK(nearColor(pixelOf(decoded.image, 13, 6), SK_ColorBLUE, 6));
  }
}

TEST_CASE("an image less than twice its box keeps every pixel") {
  const auto decoded = decodeImageNow(requestFor(testing::kJpeg, {40, 30}));

  REQUIRE(decoded.image != nullptr);
  CHECK(decoded.image->width() == 64);
  CHECK(decoded.image->height() == 48);
}

TEST_CASE("a JPEG turned by its EXIF orientation is decoded upright") {
  const auto decoded = decodeImageNow(requestFor(testing::kTurnedJpeg));
  const auto small = decodeImageNow(requestFor(testing::kTurnedJpeg, {12, 16}));

  REQUIRE(decoded.image != nullptr);
  CHECK(decoded.size == PixelSize{48, 64});
  CHECK(decoded.image->width() == 48);
  CHECK(decoded.image->height() == 64);
  CHECK(nearColor(pixelOf(decoded.image, 24, 8), SK_ColorRED, 6));
  CHECK(nearColor(pixelOf(decoded.image, 24, 56), SK_ColorBLUE, 6));
  REQUIRE(small.image != nullptr);
  CHECK(small.image->width() == 12);
  CHECK(small.image->height() == 16);
  CHECK(encodedImageSize(*imageBytes(testing::kTurnedJpeg)) == PixelSize{48, 64});
}

TEST_CASE("an image is read from a file when the request has no bytes") {
  const auto path = std::string("/tmp/rn-webos-image-decoder-test.png");
  std::ofstream(path, std::ios::binary) << *imageBytes(testing::kPng);

  const auto decoded = decodeImageNow({.bytes = nullptr, .path = path, .fit = ImageFit::Stretch, .box = {}, .naturalScale = 1});

  REQUIRE(decoded.image != nullptr);
  CHECK(decoded.size == PixelSize{64, 48});
  std::remove(path.c_str());
}

TEST_CASE("what is not an image, or not there, is an error and no picture") {
  const auto garbage = decodeImageNow({.bytes = std::make_shared<const std::string>("<html>404</html>")});
  const auto missing = decodeImageNow({.bytes = nullptr, .path = "/no/such/image.png"});
  auto cut = *imageBytes(testing::kPng);
  cut.resize(30);

  CHECK(garbage.image == nullptr);
  CHECK(garbage.error == "Not a PNG, JPEG, WebP or GIF image");
  CHECK(missing.image == nullptr);
  CHECK(missing.error == "Cannot read /no/such/image.png");
  CHECK(decodeImageNow({.bytes = std::make_shared<const std::string>(cut)}).image == nullptr);
  CHECK(encodedImageSize("not an image") == PixelSize{});
  CHECK(encodedImageSize(*imageBytes(testing::kGif)) == PixelSize{64, 48});
}

TEST_CASE("decodeImage decodes off the main thread and delivers on the main loop") {
  bool done = false;
  DecodedImage delivered;
  const auto main = std::this_thread::get_id();
  std::thread::id deliveredOn;

  const auto decode = decodeImage(requestFor(testing::kJpeg, {16, 12}), [&](DecodedImage image) {
    delivered = std::move(image);
    deliveredOn = std::this_thread::get_id();
    done = true;
  });
  CHECK_FALSE(done);
  pump(done);

  REQUIRE(done);
  CHECK(deliveredOn == main);
  REQUIRE(delivered.image != nullptr);
  CHECK(delivered.image->width() == 16);
}

TEST_CASE("a decode that was dropped delivers nothing") {
  bool delivered = false;
  bool later = false;

  decodeImage(requestFor(testing::kPng), [&](DecodedImage) { delivered = true; });
  const auto kept = decodeImage(requestFor(testing::kPng), [&](DecodedImage) { later = true; });
  pump(later);

  CHECK(later);
  CHECK_FALSE(delivered);
}

} // namespace rnwebos
