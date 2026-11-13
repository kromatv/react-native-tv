#include "BorderSides.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

constexpr auto kInRing = [](float, float) { return true; };

SideJoins joinsOf(const Edges &widths) {
  return {.widths = widths, .reach = {.left = 10, .top = 10, .right = 10, .bottom = 10}, .width = 41, .height = 31};
}

} // namespace

TEST_CASE("SideSet holds the sides it was given") {
  const SideSet sides{Side::Top, Side::Left};

  CHECK(sides.has(Side::Top));
  CHECK(sides.has(Side::Left));
  CHECK_FALSE(sides.has(Side::Right));
  CHECK_FALSE(sides.has(Side::Bottom));
  CHECK_FALSE(sides.isEmpty());
  CHECK(SideSet{}.isEmpty());
  CHECK(sides.with({Side::Right}) == SideSet{Side::Top, Side::Right, Side::Left});
  CHECK(SideSet{Side::Top, Side::Right}.with({Side::Bottom, Side::Left}) == SideSet::all());
  CHECK(sides.bits() != SideSet{Side::Top}.bits());
}

TEST_CASE("a RingPart is whole by default and once it draws four sides") {
  CHECK(RingPart{}.isWhole());
  CHECK(RingPart{.sides = SideSet::all()}.isWhole());
  CHECK_FALSE(RingPart{.sides = {Side::Top, Side::Right, Side::Bottom}}.isWhole());
}

TEST_CASE("between two corner pieces a point belongs to the side it lies along, and to none in the middle") {
  const auto joins = joinsOf({.left = 4, .top = 4, .right = 4, .bottom = 4});

  CHECK(joins.sideAt(20.5f, 0.5f) == Side::Top);
  CHECK(joins.sideAt(20.5f, 9.5f) == Side::Top);
  CHECK(joins.sideAt(20.5f, 30.5f) == Side::Bottom);
  CHECK(joins.sideAt(0.5f, 15.5f) == Side::Left);
  CHECK(joins.sideAt(40.5f, 15.5f) == Side::Right);
  CHECK(joins.sideAt(20.5f, 15.5f) == std::nullopt);
}

TEST_CASE("sides of one width join on each corner's diagonal") {
  const auto joins = joinsOf({.left = 4, .top = 4, .right = 4, .bottom = 4});

  CHECK(joins.sideAt(3.5f, 1.5f) == Side::Top);
  CHECK(joins.sideAt(1.5f, 3.5f) == Side::Left);
  CHECK(joins.sideAt(37.5f, 1.5f) == Side::Top);
  CHECK(joins.sideAt(39.5f, 3.5f) == Side::Right);
  CHECK(joins.sideAt(39.5f, 27.5f) == Side::Right);
  CHECK(joins.sideAt(37.5f, 29.5f) == Side::Bottom);
  CHECK(joins.sideAt(1.5f, 27.5f) == Side::Left);
  CHECK(joins.sideAt(3.5f, 29.5f) == Side::Bottom);
  CHECK(joins.sideAt(8.5f, 6.5f) == Side::Top);
  CHECK(joins.sideAt(6.5f, 8.5f) == Side::Left);
}

TEST_CASE("sides of two widths join on the line from the outer corner through the inner one") {
  const auto joins = joinsOf({.left = 2, .top = 8, .right = 6, .bottom = 3});

  CHECK(joins.sideAt(1.5f, 5.5f) == Side::Top);
  CHECK(joins.sideAt(1.5f, 6.5f) == Side::Left);
  CHECK(joins.sideAt(2.25f, 8.5f) == Side::Top);
  CHECK(joins.sideAt(2.0f, 8.5f) == Side::Left);
  CHECK(joins.sideAt(41 - 5.5f, 31 - 2.5f) == Side::Bottom);
  CHECK(joins.sideAt(41 - 4.5f, 31 - 2.5f) == Side::Right);
}

TEST_CASE("a side with no width leaves its corners to its neighbours") {
  const auto joins = joinsOf({.left = 5, .top = 0, .right = 5, .bottom = 5});

  CHECK(joins.sideAt(9.5f, 0.5f) == Side::Left);
  CHECK(joins.sideAt(31.5f, 0.5f) == Side::Right);
  CHECK(joins.sideAt(0.5f, 30.5f) == Side::Left);
  CHECK(joins.sideAt(9.5f, 30.5f) == Side::Bottom);
}

TEST_CASE("partCoverage counts the samples of the part's own sides") {
  const auto joins = joinsOf({.left = 4, .top = 4, .right = 4, .bottom = 4});

  CHECK(partCoverage(joins, {.sides = {Side::Top}}, 20, 2, kInRing) == 1);
  CHECK(partCoverage(joins, {.sides = {Side::Left}}, 20, 2, kInRing) == 0);
  CHECK(partCoverage(joins, {.sides = {Side::Top}}, 20, 15, kInRing) == 0);
  CHECK(partCoverage(joins, {.sides = {Side::Top}}, 20, 2, [](float, float) { return false; }) == 0);
  CHECK(partCoverage(joins, {.sides = {Side::Top}}, 20, 2, [](float x, float) { return x < 20.5f; }) == 0.5f);
}

TEST_CASE("on a join a pixel is shared between its two sides") {
  const auto joins = joinsOf({.left = 4, .top = 4, .right = 4, .bottom = 4});
  const auto top = partCoverage(joins, {.sides = {Side::Top}}, 2, 2, kInRing);
  const auto left = partCoverage(joins, {.sides = {Side::Left}}, 2, 2, kInRing);

  CHECK(top > 0);
  CHECK(left > 0);
  CHECK(top + left == 1);
  CHECK(partCoverage(joins, {.sides = {Side::Top, Side::Left}}, 2, 2, kInRing) == 1);
}

TEST_CASE("a part fills the pixels of a join with a side it runs under, and no pixel beyond") {
  const auto joins = joinsOf({.left = 4, .top = 4, .right = 4, .bottom = 4});
  const RingPart top{.sides = {Side::Top}, .under = {Side::Left}};

  CHECK(partCoverage(joins, top, 2, 2, kInRing) == 1);
  CHECK(partCoverage(joins, top, 3, 1, kInRing) == 1);
  CHECK(partCoverage(joins, top, 1, 3, kInRing) == 0);
  CHECK(partCoverage(joins, top, 0, 15, kInRing) == 0);
}

} // namespace rntizen
