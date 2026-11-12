#include "SvgViewBox.h"

#include <doctest/doctest.h>

namespace rntizen::svg {

namespace {

constexpr int kMeet = 0;
constexpr int kSlice = 1;

SvgViewBox viewBox(const char *align, int meetOrSlice) {
  return {.minX = 0, .minY = 0, .width = 100, .height = 50, .align = align, .meetOrSlice = meetOrSlice};
}

void checkPoint(const Point &actual, double x, double y) {
  CHECK(actual.x == doctest::Approx(x));
  CHECK(actual.y == doctest::Approx(y));
}

} // namespace

TEST_CASE("SvgViewBox is present only with an alignment and a positive size") {
  CHECK(viewBox("xMidYMid", kMeet).present());
  CHECK_FALSE(viewBox("", kMeet).present());
  CHECK_FALSE(SvgViewBox{.width = 0, .height = 50, .align = "xMidYMid"}.present());
}

TEST_CASE("viewBoxTransform is the identity without a viewBox") {
  const auto transform = viewBoxTransform(viewBox("", kMeet), 300, 300);

  checkPoint(transform.apply({.x = 7, .y = 9}), 7, 9);
}

TEST_CASE("meet scales to fit and centers the other axis") {
  const auto transform = viewBoxTransform(viewBox("xMidYMid", kMeet), 200, 200);

  checkPoint(transform.apply({.x = 0, .y = 0}), 0, 50);
  checkPoint(transform.apply({.x = 100, .y = 50}), 200, 150);
}

TEST_CASE("slice scales to cover and crops the other axis") {
  const auto transform = viewBoxTransform(viewBox("xMidYMid", kSlice), 200, 200);

  checkPoint(transform.apply({.x = 0, .y = 0}), -100, 0);
  checkPoint(transform.apply({.x = 100, .y = 50}), 300, 200);
}

TEST_CASE("xMinYMin and xMaxYMax pin the viewBox to a corner") {
  checkPoint(viewBoxTransform(viewBox("xMinYMin", kMeet), 200, 200).apply({.x = 0, .y = 0}), 0, 0);
  checkPoint(viewBoxTransform(viewBox("xMaxYMax", kMeet), 200, 200).apply({.x = 100, .y = 50}), 200, 200);
}

TEST_CASE("align none stretches each axis on its own") {
  const auto transform = viewBoxTransform(viewBox("none", kMeet), 200, 200);

  checkPoint(transform.apply({.x = 100, .y = 50}), 200, 200);
}

TEST_CASE("the viewBox origin moves to the view's origin") {
  auto shifted = viewBox("xMinYMin", kMeet);
  shifted.minX = 10;
  shifted.minY = 20;

  checkPoint(viewBoxTransform(shifted, 100, 50).apply({.x = 10, .y = 20}), 0, 0);
}

} // namespace rntizen::svg
