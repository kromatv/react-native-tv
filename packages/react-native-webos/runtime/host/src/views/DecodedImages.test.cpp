#include "DecodedImages.h"

#include "SampleImages.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

DecodedImage decoded(PixelSize box) {
  return decodeImageNow(
      {.bytes = testing::imageBytes(testing::kPng), .path = {}, .fit = ImageFit::Cover, .box = box, .naturalScale = 1});
}

} // namespace

TEST_CASE("a kept image is found again by its key for a box it is sharp enough for") {
  clearDecodedImages();
  Scene scene(PixelSize{100, 100});

  const auto kept = keepDecodedImage(scene, "poster", decoded({16, 12}));

  REQUIRE(kept.image != nullptr);
  const auto found = findDecodedImage("poster", ImageFit::Cover, {16, 12}, 1);
  REQUIRE(found.has_value());
  CHECK(found->image == kept.image);
  CHECK(found->size == PixelSize{64, 48});
  CHECK(findDecodedImage("poster", ImageFit::Cover, {8, 6}, 1).has_value());
  CHECK_FALSE(findDecodedImage("other", ImageFit::Cover, {16, 12}, 1).has_value());
}

TEST_CASE("an image decoded small is not found for a box that needs more of it, a whole one always is") {
  clearDecodedImages();
  Scene scene(PixelSize{100, 100});
  keepDecodedImage(scene, "small", decoded({16, 12}));
  keepDecodedImage(scene, "whole", decoded({}));

  CHECK_FALSE(findDecodedImage("small", ImageFit::Cover, {64, 48}, 1).has_value());
  CHECK_FALSE(findDecodedImage("small", ImageFit::Cover, {20, 15}, 1).has_value());
  CHECK(findDecodedImage("whole", ImageFit::Cover, {64, 48}, 1).has_value());
  CHECK(findDecodedImage("whole", ImageFit::Cover, {640, 480}, 1).has_value());
}

TEST_CASE("of the sizes kept of one image, a box gets one that is sharp enough for it") {
  clearDecodedImages();
  Scene scene(PixelSize{100, 100});
  const auto small = keepDecodedImage(scene, "poster", decoded({16, 12}));
  const auto whole = keepDecodedImage(scene, "poster", decoded({}));

  CHECK(findDecodedImage("poster", ImageFit::Cover, {64, 48}, 1)->image == whole.image);
  CHECK(findDecodedImage("poster", ImageFit::Cover, {30, 20}, 1)->image == whole.image);
}

TEST_CASE("clearDecodedImages forgets every image, and a failed decode is never kept") {
  Scene scene(PixelSize{100, 100});
  keepDecodedImage(scene, "poster", decoded({}));
  keepDecodedImage(scene, "broken", DecodedImage{.image = nullptr, .size = {}, .error = "no"});

  CHECK(findDecodedImage("poster", ImageFit::Cover, {16, 12}, 1).has_value());
  CHECK_FALSE(findDecodedImage("broken", ImageFit::Cover, {16, 12}, 1).has_value());
  clearDecodedImages();
  CHECK_FALSE(findDecodedImage("poster", ImageFit::Cover, {16, 12}, 1).has_value());
}

} // namespace rnwebos
