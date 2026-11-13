#include "BoxBorder.h"

#include "ViewFixtures.test.h"
#include "ViewSmartObject.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <vector>

namespace rntizen {

namespace {

using facebook::react::BorderColors;
using facebook::react::colorFromRGBA;

constexpr int kSize = 100;
constexpr Corners kRound{.topLeft = {20, 20}, .topRight = {20, 20}, .bottomRight = {20, 20}, .bottomLeft = {20, 20}};
constexpr Edges kWidths{.left = 4, .top = 4, .right = 4, .bottom = 4};

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kGreen = colorFromRGBA(0, 255, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);
const auto kYellow = colorFromRGBA(255, 255, 0, 255);

/// A border of its own on a smart object, to look at what it draws with.
struct Bench {
  std::vector<Evas_Object *> objectsOf(const Corners &corners, const BorderColors &colors) {
    border.update({.corners = corners, .borderWidths = kWidths, .borderColors = colors});
    border.layout({.x = 0, .y = 0, .width = kSize, .height = kSize});
    return border.objects();
  }

  const testing::TestCanvas canvas{kSize};
  BoxBorder border{canvas.evas(), addViewSmartObject(canvas.evas())};
};

} // namespace

TEST_CASE("one color is one image holding the ring's patch") {
  Bench bench;

  const auto objects = bench.objectsOf(kRound, {.left = kRed, .top = kRed, .right = kRed, .bottom = kRed});

  REQUIRE(objects.size() == 1);
  const auto ring = ringPatch(kRound, kWidths);
  int width = 0;
  int height = 0;
  evas_object_image_size_get(objects[0], &width, &height);
  REQUIRE(width == ring.width);
  REQUIRE(height == ring.height);
  auto *pixels = static_cast<std::uint32_t *>(evas_object_image_data_get(objects[0], EINA_FALSE));
  CHECK(std::equal(ring.pixels.begin(), ring.pixels.end(), pixels));
  evas_object_image_data_set(objects[0], pixels);
  Edges border;
  evas_object_image_border_get(objects[0], &border.left, &border.right, &border.top, &border.bottom);
  CHECK(border == ring.border);
  CHECK(evas_object_image_border_center_fill_get(objects[0]) == EVAS_BORDER_FILL_NONE);
}

TEST_CASE("a rounded border keeps one image per color, and none for a transparent side") {
  Bench bench;

  CHECK(bench.objectsOf(kRound, {.top = kRed, .right = kRed}).size() == 1);
  CHECK(bench.objectsOf(kRound, {.top = kRed, .right = kGreen}).size() == 2);
  CHECK(bench.objectsOf(kRound, {.left = kYellow, .top = kRed, .right = kGreen, .bottom = kBlue}).size() == 4);
  CHECK(bench.objectsOf(kRound, {.left = kGreen, .top = kRed, .right = kGreen, .bottom = kRed}).size() == 2);
  CHECK(bench.objectsOf(kRound, {}).empty());
}

TEST_CASE("a change of color alone keeps the images, a change of their number says so") {
  Bench bench;
  const BoxStyle half{.corners = kRound, .borderWidths = kWidths, .borderColors = {.top = kRed, .right = kRed}};
  auto recolored = half;
  recolored.borderColors.top = kBlue;
  recolored.borderColors.right = kBlue;
  auto split = half;
  split.borderColors.right = kGreen;

  CHECK(bench.border.update(half));
  CHECK_FALSE(bench.border.update(recolored));
  CHECK(bench.border.update(split));
}

TEST_CASE("a square box with a color per side keeps its four rectangles") {
  Bench bench;

  const auto objects = bench.objectsOf(Corners{}, {.left = kYellow, .top = kRed, .right = kGreen, .bottom = kBlue});

  REQUIRE(objects.size() == 4);
  for (const auto *object : objects) CHECK(std::string_view{evas_object_type_get(object)} == "rectangle");
}

} // namespace rntizen
