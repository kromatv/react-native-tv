#include "VideoFit.h"

#include <doctest/doctest.h>

namespace rntizen::expo {

namespace {

const PixelRect kScreen{.x = 0, .y = 0, .width = 1920, .height = 1080};

} // namespace

TEST_CASE("contain letterboxes the frame in the view and shows all of it") {
  const auto plane = fitVideo({.x = 0, .y = 0, .width = 100, .height = 100}, kScreen, 200, 100, ContentFit::Contain);
  REQUIRE(plane.has_value());
  CHECK(plane->roi == PixelRect{.x = 0, .y = 25, .width = 100, .height = 50});
  CHECK(plane->crop == SourceCrop{});
}

TEST_CASE("cover fills the view and crops the frame to its middle") {
  const auto plane = fitVideo({.x = 0, .y = 0, .width = 100, .height = 100}, kScreen, 200, 100, ContentFit::Cover);
  REQUIRE(plane.has_value());
  CHECK(plane->roi == PixelRect{.x = 0, .y = 0, .width = 100, .height = 100});
  CHECK(plane->crop == SourceCrop{.x = 0.25, .y = 0, .width = 0.5, .height = 1});
}

TEST_CASE("fill and an unknown video size stretch the frame over the view") {
  const PixelRect view{.x = 10, .y = 20, .width = 300, .height = 200};
  const PlaneGeometry whole{.roi = view, .crop = {}};
  CHECK(fitVideo(view, kScreen, 1920, 1080, ContentFit::Fill) == whole);
  CHECK(fitVideo(view, kScreen, 0, 0, ContentFit::Contain) == whole);
  CHECK(fitVideo(view, kScreen, 1920, -1, ContentFit::Cover) == whole);
}

TEST_CASE("the part of the view outside the window is cropped away") {
  const auto plane =
      fitVideo({.x = 1800, .y = 0, .width = 240, .height = 135}, kScreen, 1920, 1080, ContentFit::Fill);
  REQUIRE(plane.has_value());
  CHECK(plane->roi == PixelRect{.x = 1800, .y = 0, .width = 120, .height = 135});
  CHECK(plane->crop == SourceCrop{.x = 0, .y = 0, .width = 0.5, .height = 1});
}

TEST_CASE("nothing is shown for a view off the window or too small to draw") {
  CHECK_FALSE(fitVideo({.x = 2000, .y = 0, .width = 100, .height = 100}, kScreen, 16, 9, ContentFit::Fill).has_value());
  CHECK_FALSE(fitVideo({.x = 0, .y = 0, .width = 0, .height = 100}, kScreen, 16, 9, ContentFit::Contain).has_value());
  CHECK_FALSE(fitVideo({.x = 0, .y = 0, .width = 100, .height = 100}, kScreen, 1000, 1, ContentFit::Contain).has_value());
}

} // namespace rntizen::expo
