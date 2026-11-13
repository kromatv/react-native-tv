#include "BorderLayers.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

using facebook::react::BorderColors;
using facebook::react::colorFromRGBA;

constexpr Edges kEven{.left = 3, .top = 3, .right = 3, .bottom = 3};

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kGreen = colorFromRGBA(0, 255, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);
const auto kYellow = colorFromRGBA(255, 255, 0, 255);
const auto kFaintRed = colorFromRGBA(255, 0, 0, 77);
const auto kClear = colorFromRGBA(0, 0, 0, 0);

} // namespace

TEST_CASE("one color on every side is the whole ring, as one layer") {
  const auto layers = borderLayers(kEven, {.left = kRed, .top = kRed, .right = kRed, .bottom = kRed});

  REQUIRE(layers.size() == 1);
  CHECK(layers[0].part.isWhole());
  CHECK(layers[0].part.under.isEmpty());
  CHECK(layers[0].color == kRed);
}

TEST_CASE("the color of a side with no width does not split the ring") {
  const Edges widths{.left = 3, .top = 0, .right = 3, .bottom = 3};

  const auto layers = borderLayers(widths, {.left = kRed, .top = kBlue, .right = kRed, .bottom = kRed});

  REQUIRE(layers.size() == 1);
  CHECK(layers[0].part.isWhole());
  CHECK(layers[0].color == kRed);
}

TEST_CASE("no width or no visible color draws no layer") {
  CHECK(borderLayers({}, {.left = kRed, .top = kRed, .right = kRed, .bottom = kRed}).empty());
  CHECK(borderLayers(kEven, {.left = kClear, .top = kClear, .right = kClear, .bottom = kClear}).empty());
  CHECK(borderLayers(kEven, BorderColors{}).empty());
}

TEST_CASE("two colored sides beside two transparent ones are one layer of those two sides") {
  const auto layers = borderLayers(kEven, {.left = kClear, .top = kRed, .right = kRed, .bottom = kClear});

  REQUIRE(layers.size() == 1);
  CHECK(layers[0].part.sides == SideSet{Side::Top, Side::Right});
  CHECK(layers[0].part.under.isEmpty());
  CHECK_FALSE(layers[0].part.isWhole());
  CHECK(layers[0].color == kRed);
}

TEST_CASE("an unset color leaves its side out") {
  const auto layers = borderLayers(kEven, {.left = kRed, .top = kRed, .right = kRed, .bottom = {}});

  REQUIRE(layers.size() == 1);
  CHECK(layers[0].part.sides == SideSet{Side::Top, Side::Right, Side::Left});
}

TEST_CASE("four colors are four layers, each under the opaque ones above it") {
  const auto layers = borderLayers(kEven, {.left = kYellow, .top = kRed, .right = kGreen, .bottom = kBlue});

  REQUIRE(layers.size() == 4);
  CHECK(layers[0] == BorderLayer{.part = {.sides = {Side::Top}, .under = {Side::Right, Side::Bottom, Side::Left}}, .color = kRed});
  CHECK(layers[1] == BorderLayer{.part = {.sides = {Side::Right}, .under = {Side::Bottom, Side::Left}}, .color = kGreen});
  CHECK(layers[2] == BorderLayer{.part = {.sides = {Side::Bottom}, .under = {Side::Left}}, .color = kBlue});
  CHECK(layers[3] == BorderLayer{.part = {.sides = {Side::Left}}, .color = kYellow});
}

TEST_CASE("sides of one color share a layer") {
  const auto layers = borderLayers(kEven, {.left = kBlue, .top = kRed, .right = kBlue, .bottom = kRed});

  REQUIRE(layers.size() == 2);
  CHECK(layers[0] == BorderLayer{.part = {.sides = {Side::Top, Side::Bottom}, .under = {Side::Right, Side::Left}}, .color = kRed});
  CHECK(layers[1] == BorderLayer{.part = {.sides = {Side::Right, Side::Left}}, .color = kBlue});
}

TEST_CASE("translucent layers come first and run under the opaque ones only") {
  const auto layers = borderLayers(kEven, {.left = kFaintRed, .top = kRed, .right = kBlue, .bottom = kFaintRed});

  REQUIRE(layers.size() == 3);
  CHECK(layers[0] == BorderLayer{.part = {.sides = {Side::Bottom, Side::Left}, .under = {Side::Top, Side::Right}}, .color = kFaintRed});
  CHECK(layers[1] == BorderLayer{.part = {.sides = {Side::Top}, .under = {Side::Right}}, .color = kRed});
  CHECK(layers[2] == BorderLayer{.part = {.sides = {Side::Right}}, .color = kBlue});
}

TEST_CASE("a translucent layer runs under no other translucent one") {
  const auto faintBlue = colorFromRGBA(0, 0, 255, 128);

  const auto layers = borderLayers(kEven, {.left = faintBlue, .top = kFaintRed, .right = faintBlue, .bottom = kFaintRed});

  REQUIRE(layers.size() == 2);
  CHECK(layers[0].color == kFaintRed);
  CHECK(layers[0].part.under.isEmpty());
  CHECK(layers[1].color == faintBlue);
  CHECK(layers[1].part.under.isEmpty());
}

} // namespace rntizen
