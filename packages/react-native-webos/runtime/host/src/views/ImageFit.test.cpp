#include "ImageFit.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

constexpr PixelRect kBox{.x = 10, .y = 20, .width = 200, .height = 100};

} // namespace

TEST_CASE("stretch paints the whole image over the whole box") {
  const auto placed = placeImage(ImageFit::Stretch, kBox, {50, 400}, 1);

  CHECK(placed.destination == SkRect::MakeXYWH(10, 20, 200, 100));
  CHECK(placed.source == SkRect::MakeWH(50, 400));
  CHECK(placed.tile.isEmpty());
}

TEST_CASE("contain scales the image evenly to fit inside the box, centered") {
  const auto wide = placeImage(ImageFit::Contain, kBox, {400, 100}, 1);
  const auto tall = placeImage(ImageFit::Contain, kBox, {100, 400}, 1);

  CHECK(wide.destination == SkRect::MakeXYWH(10, 45, 200, 50));
  CHECK(wide.source == SkRect::MakeWH(400, 100));
  CHECK(tall.destination == SkRect::MakeXYWH(97, 20, 25, 100));
}

TEST_CASE("cover fills the box with the middle of the image and leaves the overflow unpainted") {
  const auto wide = placeImage(ImageFit::Cover, kBox, {400, 100}, 1);
  const auto tall = placeImage(ImageFit::Cover, kBox, {100, 400}, 1);

  CHECK(wide.destination == SkRect::MakeXYWH(10, 20, 200, 100));
  CHECK(wide.source == SkRect::MakeLTRB(100, 0, 300, 100));
  CHECK(tall.destination == SkRect::MakeXYWH(10, 20, 200, 100));
  CHECK(tall.source == SkRect::MakeLTRB(0, 175, 100, 225));
}

TEST_CASE("center keeps a small image at its natural size and scales a large one down to fit") {
  const auto small = placeImage(ImageFit::Center, kBox, {40, 20}, 2);
  const auto large = placeImage(ImageFit::Center, kBox, {400, 400}, 2);

  CHECK(small.destination == SkRect::MakeXYWH(70, 50, 80, 40));
  CHECK(large.destination == SkRect::MakeXYWH(60, 20, 100, 100));
  CHECK(large.source == SkRect::MakeWH(400, 400));
}

TEST_CASE("natural never scales: a large image shows its middle") {
  const auto placed = placeImage(ImageFit::Natural, kBox, {400, 300}, 1);

  CHECK(placed.destination == SkRect::MakeXYWH(10, 20, 200, 100));
  CHECK(placed.source == SkRect::MakeLTRB(100, 100, 300, 200));
}

TEST_CASE("repeat tiles copies at the natural size from the box's corner") {
  const auto placed = placeImage(ImageFit::Repeat, kBox, {8, 8}, 2);

  CHECK(placed.destination == SkRect::MakeXYWH(10, 20, 200, 100));
  CHECK(placed.tile == SkSize::Make(16, 16));
}

TEST_CASE("an image or a box with no area is placed over the box as it is") {
  CHECK(placeImage(ImageFit::Cover, kBox, {0, 0}, 1).destination == SkRect::MakeXYWH(10, 20, 200, 100));
  CHECK(placeImage(ImageFit::Cover, {.x = 0, .y = 0, .width = 0, .height = 50}, {10, 10}, 1).destination.isEmpty());
}

TEST_CASE("an image is decoded at the size it is drawn at once it is twice that, and at its own size otherwise") {
  CHECK(decodedImageSize(ImageFit::Cover, {200, 100}, {4000, 3000}, 1) == PixelSize{200, 150});
  CHECK(decodedImageSize(ImageFit::Contain, {200, 100}, {4000, 3000}, 1) == PixelSize{133, 100});
  CHECK(decodedImageSize(ImageFit::Stretch, {200, 100}, {4000, 3000}, 1) == PixelSize{200, 100});
  CHECK(decodedImageSize(ImageFit::Cover, {200, 100}, {300, 250}, 1) == PixelSize{300, 250});
  CHECK(decodedImageSize(ImageFit::Stretch, {200, 100}, {4000, 150}, 1) == PixelSize{4000, 150});
  CHECK(decodedImageSize(ImageFit::Center, {200, 100}, {4000, 3000}, 1) == PixelSize{133, 100});
  CHECK(decodedImageSize(ImageFit::Center, {200, 100}, {40, 30}, 1) == PixelSize{40, 30});
  CHECK(decodedImageSize(ImageFit::Repeat, {200, 100}, {4000, 3000}, 1) == PixelSize{4000, 3000});
  CHECK(decodedImageSize(ImageFit::Natural, {200, 100}, {4000, 3000}, 1) == PixelSize{4000, 3000});
  CHECK(decodedImageSize(ImageFit::Cover, {0, 0}, {4000, 3000}, 1) == PixelSize{4000, 3000});
}

} // namespace rnwebos
