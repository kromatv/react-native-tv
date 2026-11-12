#include "SvgViewBox.h"

#include "include/core/SkPoint.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

namespace {

constexpr int kMeet = 0;
constexpr int kSlice = 1;

SvgViewBox viewBox(const char *align, int meetOrSlice) {
  return {.minX = 0, .minY = 0, .width = 100, .height = 50, .align = align, .meetOrSlice = meetOrSlice};
}

void checkPoint(SkPoint actual, float x, float y) {
  CHECK(actual.x() == doctest::Approx(x));
  CHECK(actual.y() == doctest::Approx(y));
}

} // namespace

TEST_CASE("SvgViewBox is present only with an alignment and a positive size") {
  CHECK(viewBox("xMidYMid", kMeet).present());
  CHECK_FALSE(viewBox("", kMeet).present());
  CHECK_FALSE(SvgViewBox{.width = 0, .height = 50, .align = "xMidYMid"}.present());
}

TEST_CASE("viewBoxTransform is the identity without a viewBox") {
  const auto transform = viewBoxTransform(viewBox("", kMeet), 300, 300);

  checkPoint(transform.mapPoint({7, 9}), 7, 9);
}

TEST_CASE("meet scales to fit and centers the other axis") {
  const auto transform = viewBoxTransform(viewBox("xMidYMid", kMeet), 200, 200);

  checkPoint(transform.mapPoint({0, 0}), 0, 50);
  checkPoint(transform.mapPoint({100, 50}), 200, 150);
}

TEST_CASE("slice scales to cover and crops the other axis") {
  const auto transform = viewBoxTransform(viewBox("xMidYMid", kSlice), 200, 200);

  checkPoint(transform.mapPoint({0, 0}), -100, 0);
  checkPoint(transform.mapPoint({100, 50}), 300, 200);
}

TEST_CASE("xMinYMin and xMaxYMax pin the viewBox to a corner") {
  checkPoint(viewBoxTransform(viewBox("xMinYMin", kMeet), 200, 200).mapPoint({0, 0}), 0, 0);
  checkPoint(viewBoxTransform(viewBox("xMaxYMax", kMeet), 200, 200).mapPoint({100, 50}), 200, 200);
}

TEST_CASE("align none stretches each axis on its own") {
  const auto transform = viewBoxTransform(viewBox("none", kMeet), 200, 200);

  checkPoint(transform.mapPoint({100, 50}), 200, 200);
}

TEST_CASE("the viewBox origin moves to the view's origin") {
  auto shifted = viewBox("xMinYMin", kMeet);
  shifted.minX = 10;
  shifted.minY = 20;

  checkPoint(viewBoxTransform(shifted, 100, 50).mapPoint({10, 20}), 0, 0);
}

} // namespace rnwebos::svg
