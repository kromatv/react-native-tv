#include "LogicalScreen.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("pointScaleFactor is the largest scale at which the logical screen fits the window") {
  CHECK(pointScaleFactor({.width = 1280, .height = 720}) == doctest::Approx(2.0 / 3.0));
  CHECK(pointScaleFactor({.width = 1920, .height = 1080}) == 1);
  CHECK(pointScaleFactor({.width = 3840, .height = 2160}) == 2);
  CHECK(pointScaleFactor({.width = 1920, .height = 1200}) == 1);
  CHECK(pointScaleFactor({.width = 2560, .height = 1080}) == 1);
}

TEST_CASE("pointScaleFactor is one pixel per point in a window with no area") {
  CHECK(pointScaleFactor({.width = 0, .height = 0}) == 1);
  CHECK(pointScaleFactor({.width = 1920, .height = 0}) == 1);
  CHECK(pointScaleFactor({.width = -1, .height = 1080}) == 1);
}

} // namespace rntizen
