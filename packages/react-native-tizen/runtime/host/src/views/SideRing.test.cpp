#include "SideRing.h"

#include <doctest/doctest.h>

#include <cstdint>

namespace rntizen {

namespace {

constexpr Corners kCircle{.topLeft = {20, 20}, .topRight = {20, 20}, .bottomRight = {20, 20}, .bottomLeft = {20, 20}};
constexpr Edges kThin{.left = 4, .top = 4, .right = 4, .bottom = 4};
constexpr Corners kMixed{.topLeft = {12, 12}, .topRight = {30, 18}, .bottomRight = {0, 0}, .bottomLeft = {6, 24}};
constexpr Edges kUneven{.left = 3, .top = 9, .right = 5, .bottom = 2};

std::uint32_t alphaAt(const NinePatch &patch, int x, int y) {
  return patch.pixels[static_cast<std::size_t>(y * patch.width + x)] >> 24;
}

RingPart only(Side side) {
  return {.sides = {side}};
}

/// The index in the ring's patch of what `index` shows in a side patch, whose
/// corner pieces are a pixel of straight border larger.
int ringIndex(int index, int before) {
  if (index < before) return index;
  return index > before + 2 ? index - 2 : before;
}

/// The alpha of the four single-side patches added up, pixel by pixel, and the
/// ring's own alpha there.
void checkSidesAddUpToTheRing(const Corners &corners, const Edges &widths) {
  const auto ring = ringPatch(corners, widths);
  const auto top = sideRingPatch(corners, widths, only(Side::Top));
  const auto right = sideRingPatch(corners, widths, only(Side::Right));
  const auto bottom = sideRingPatch(corners, widths, only(Side::Bottom));
  const auto left = sideRingPatch(corners, widths, only(Side::Left));
  REQUIRE(top.width == ring.width + 2);
  REQUIRE(top.height == ring.height + 2);
  int differing = 0;
  for (int y = 0; y < top.height; ++y) {
    for (int x = 0; x < top.width; ++x) {
      const auto sum = alphaAt(top, x, y) + alphaAt(right, x, y) + alphaAt(bottom, x, y) + alphaAt(left, x, y);
      const auto whole = alphaAt(ring, ringIndex(x, ring.border.left), ringIndex(y, ring.border.top));
      differing += sum > whole + 1 || whole > sum + 1 ? 1 : 0;
    }
  }
  CHECK(differing == 0);
}

} // namespace

TEST_CASE("sideRingPatch is hollow, with corner pieces one pixel larger than the ring's") {
  const auto &patch = sideRingPatch(kMixed, kUneven, only(Side::Top));

  CHECK(patch.hollow);
  CHECK(patch.border == Edges{.left = 13, .top = 19, .right = 31, .bottom = 25});
  CHECK(patch.width == 13 + 31 + 1);
  CHECK(patch.height == 19 + 25 + 1);
  CHECK(patch.pixels.size() == static_cast<std::size_t>(patch.width * patch.height));
}

TEST_CASE("the four sides of a border add up to its ring") {
  checkSidesAddUpToTheRing(kCircle, kThin);
  checkSidesAddUpToTheRing(kMixed, kUneven);
  checkSidesAddUpToTheRing(Corners{}, kUneven);
  checkSidesAddUpToTheRing(kCircle, {.left = 0, .top = 6, .right = 30, .bottom = 1});
}

TEST_CASE("a side's patch draws that side and leaves the three others clear") {
  const auto &top = sideRingPatch(kCircle, kThin, only(Side::Top));
  const auto middleX = top.border.left;
  const auto middleY = top.border.top;

  CHECK(alphaAt(top, middleX, 0) == 255);
  CHECK(alphaAt(top, middleX, 3) == 255);
  CHECK(alphaAt(top, middleX, 4) == 0);
  CHECK(alphaAt(top, 0, middleY) == 0);
  CHECK(alphaAt(top, top.width - 1, middleY) == 0);
  CHECK(alphaAt(top, middleX, top.height - 1) == 0);
}

TEST_CASE("on a circle two sides meet on the diagonal of their corner") {
  const auto top = sideRingPatch(kCircle, kThin, only(Side::Top));
  const auto left = sideRingPatch(kCircle, kThin, only(Side::Left));

  CHECK(alphaAt(top, 11, 4) == 255);
  CHECK(alphaAt(left, 11, 4) == 0);
  CHECK(alphaAt(top, 4, 11) == 0);
  CHECK(alphaAt(left, 4, 11) == 255);
  CHECK(alphaAt(top, 7, 7) > 0);
  CHECK(alphaAt(left, 7, 7) > 0);
  CHECK(alphaAt(top, 7, 7) + alphaAt(left, 7, 7) == 255);
}

TEST_CASE("sides of two widths meet on the line from the outer corner through the inner one") {
  const Corners round{.topLeft = {30, 30}, .topRight = {30, 30}, .bottomRight = {30, 30}, .bottomLeft = {30, 30}};
  const Edges widths{.left = 5, .top = 20, .right = 5, .bottom = 5};
  const auto top = sideRingPatch(round, widths, only(Side::Top));
  const auto left = sideRingPatch(round, widths, only(Side::Left));

  CHECK(alphaAt(top, 9, 22) == 255);
  CHECK(alphaAt(left, 9, 22) == 0);
  CHECK(alphaAt(top, 3, 26) == 0);
  CHECK(alphaAt(left, 3, 26) == 255);
}

TEST_CASE("a square corner beside a round one is cut from corner to corner") {
  const Corners corners{.topLeft = {0, 0}, .topRight = {16, 16}, .bottomRight = {0, 0}, .bottomLeft = {0, 0}};
  const Edges widths{.left = 6, .top = 6, .right = 6, .bottom = 6};
  const auto top = sideRingPatch(corners, widths, only(Side::Top));
  const auto left = sideRingPatch(corners, widths, only(Side::Left));

  CHECK(alphaAt(top, 4, 1) == 255);
  CHECK(alphaAt(left, 4, 1) == 0);
  CHECK(alphaAt(top, 1, 4) == 0);
  CHECK(alphaAt(left, 1, 4) == 255);
  CHECK(alphaAt(top, 6, 5) == 255);
  CHECK(alphaAt(left, 5, 6) == 255);
}

TEST_CASE("a part fills the pixels of a join with a side it runs under") {
  const auto alone = sideRingPatch(kCircle, kThin, only(Side::Top));
  const auto under = sideRingPatch(kCircle, kThin, {.sides = {Side::Top}, .under = {Side::Left}});

  CHECK(alphaAt(alone, 7, 7) < 255);
  CHECK(alphaAt(under, 7, 7) == 255);
  CHECK(alphaAt(under, 11, 4) == 255);
  CHECK(alphaAt(under, 4, 11) == 0);
  CHECK(alphaAt(under, under.width - 8, 7) == alphaAt(alone, alone.width - 8, 7));
}

TEST_CASE("sideRingPatch pixels are white premultiplied by their coverage") {
  const auto &patch = sideRingPatch(kMixed, kUneven, {.sides = {Side::Top, Side::Right}});

  for (const auto pixel : patch.pixels) {
    const auto alpha = pixel >> 24;
    CHECK(pixel == (alpha << 24 | alpha << 16 | alpha << 8 | alpha));
  }
}

} // namespace rntizen
