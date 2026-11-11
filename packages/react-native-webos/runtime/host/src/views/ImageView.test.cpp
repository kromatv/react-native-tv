#include "ImageView.h"

#include "../platform/MainLoop.h"
#include "../render/RasterScene.test.h"
#include "DecodedImages.h"
#include "SampleImages.test.h"
#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

#include <react/renderer/components/image/ImageProps.h>

#include <chrono>
#include <cstdio>
#include <fstream>
#include <functional>
#include <string>
#include <thread>

namespace rnwebos {

namespace {

using facebook::react::ImageProps;
using facebook::react::ImageResizeMode;
using testing::frameOf;
using testing::nearColor;
using testing::place;
using testing::propsWith;

facebook::react::Props::Shared showing(const std::string &uri, ImageResizeMode resizeMode = ImageResizeMode::Stretch) {
  return propsWith<ImageProps>([&](ImageProps &props) {
    facebook::react::ImageSource source;
    source.type = facebook::react::ImageSource::Type::Remote;
    source.uri = uri;
    props.sources = {source};
    props.resizeMode = resizeMode;
  });
}

std::string dataUri(std::string_view base64) {
  return "data:image/png;base64," + std::string(base64);
}

struct Stage {
  bool pumpUntil(const std::function<bool()> &done) {
    const auto giveUp = MainLoop::Clock::now() + std::chrono::seconds(5);
    while (MainLoop::Clock::now() < giveUp) {
      MainLoop::get().iterate();
      frame.draw();
      if (done()) return true;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return false;
  }

  bool painted() const {
    return frame.pixel(30, 30) != SK_ColorBLACK;
  }

  testing::RasterScene frame{PixelSize{200, 100}};
  std::unique_ptr<ComponentView> root{testing::rootOf(frame.scene())};
};

} // namespace

TEST_CASE("an Image decodes a data: URI off the main thread, then paints it fitted in its box") {
  clearDecodedImages();
  Stage stage;
  ImageView image(stage.frame.scene(), 2);
  place(image, showing(dataUri(testing::kPng)), frameOf(20, 10, 100, 50));
  stage.root->mountChild(image, 0);

  stage.frame.draw();
  CHECK_FALSE(stage.painted());
  REQUIRE(stage.pumpUntil([&] { return stage.painted(); }));

  CHECK(nearColor(stage.frame.pixel(30, 30), SK_ColorRED));
  CHECK(nearColor(stage.frame.pixel(110, 30), SK_ColorBLUE));
  CHECK(stage.frame.pixel(125, 30) == SK_ColorBLACK);
  CHECK(stage.frame.pixel(30, 65) == SK_ColorBLACK);
}

TEST_CASE("an Image that mounts again with a picture kept in memory paints it in its first frame") {
  clearDecodedImages();
  Stage stage;
  {
    ImageView first(stage.frame.scene(), 2);
    place(first, showing(dataUri(testing::kPng)), frameOf(20, 10, 100, 50));
    stage.root->mountChild(first, 0);
    REQUIRE(stage.pumpUntil([&] { return stage.painted(); }));
  }
  stage.frame.draw();
  CHECK_FALSE(stage.painted());

  ImageView second(stage.frame.scene(), 3);
  place(second, showing(dataUri(testing::kPng)), frameOf(20, 10, 100, 50));
  stage.root->mountChild(second, 0);
  stage.frame.draw();

  CHECK(nearColor(stage.frame.pixel(30, 30), SK_ColorRED));
}

TEST_CASE("an Image reads a file source, covers its box with it and tints it") {
  clearDecodedImages();
  const auto path = std::string("/tmp/rn-webos-image-view-test.png");
  std::ofstream(path, std::ios::binary) << *testing::imageBytes(testing::kPng);
  Stage stage;
  ImageView image(stage.frame.scene(), 2);
  place(image, showing("file://" + path, ImageResizeMode::Cover), frameOf(20, 10, 100, 50));
  stage.root->mountChild(image, 0);
  REQUIRE(stage.pumpUntil([&] { return stage.painted(); }));

  CHECK(nearColor(stage.frame.pixel(30, 30), SK_ColorRED));
  CHECK(nearColor(stage.frame.pixel(110, 30), SK_ColorBLUE));

  image.updateProps(propsWith<ImageProps>([&](ImageProps &props) {
    facebook::react::ImageSource source;
    source.type = facebook::react::ImageSource::Type::Remote;
    source.uri = "file://" + path;
    props.sources = {source};
    props.tintColor = facebook::react::colorFromRGBA(0, 255, 0, 255);
  }));
  stage.frame.draw();
  CHECK(nearColor(stage.frame.pixel(30, 30), SK_ColorGREEN));
  CHECK(nearColor(stage.frame.pixel(110, 30), SK_ColorGREEN));
  std::remove(path.c_str());
}

TEST_CASE("an Image shows nothing for a source that is not an image, and drops its picture with its source") {
  clearDecodedImages();
  Stage stage;
  ImageView image(stage.frame.scene(), 2);
  place(image, showing("data:text/plain;base64,bm90IGFuIGltYWdl"), frameOf(20, 10, 100, 50));
  stage.root->mountChild(image, 0);
  CHECK_FALSE(stage.pumpUntil([&] { return stage.painted(); }));

  image.updateProps(showing(dataUri(testing::kPng)));
  REQUIRE(stage.pumpUntil([&] { return stage.painted(); }));

  image.updateProps(std::make_shared<ImageProps>());
  stage.frame.draw();
  CHECK_FALSE(stage.painted());
}

TEST_CASE("an Image keeps its picture inside its padding and behind its rounded corners") {
  clearDecodedImages();
  Stage stage;
  ImageView image(stage.frame.scene(), 2);
  auto layout = frameOf(20, 10, 100, 50);
  layout.contentInsets = {.left = 10, .top = 5, .right = 10, .bottom = 5};
  place(image, showing(dataUri(testing::kPng)), layout);
  stage.root->mountChild(image, 0);
  REQUIRE(stage.pumpUntil([&] { return stage.frame.pixel(40, 30) != SK_ColorBLACK; }));

  CHECK(stage.frame.pixel(25, 30) == SK_ColorBLACK);
  CHECK(stage.frame.pixel(70, 12) == SK_ColorBLACK);
  CHECK(nearColor(stage.frame.pixel(40, 30), SK_ColorRED));
}

} // namespace rnwebos
