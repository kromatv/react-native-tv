#include "PathData.h"

#include "include/core/SkPathTypes.h"

#include <doctest/doctest.h>

#include <vector>

namespace rnwebos::svg {

namespace {

using Verb = SkPathVerb;

void checkPoint(SkPoint actual, float x, float y) {
  CHECK(actual.x() == doctest::Approx(x));
  CHECK(actual.y() == doctest::Approx(y));
}

std::vector<Verb> verbs(const SkPath &path) {
  const auto found = path.verbs();
  return {found.begin(), found.end()};
}

SkPoint lastPoint(const SkPath &path) {
  return path.getLastPt().value_or(SkPoint{-1, -1});
}

} // namespace

TEST_CASE("parsePathData reads absolute moves, lines and a close") {
  const auto path = parsePathData("M10 20 L30 40 Z");

  REQUIRE(verbs(path) == std::vector{Verb::kMove, Verb::kLine, Verb::kClose});
  checkPoint(path.getPoint(0), 10, 20);
  checkPoint(path.getPoint(1), 30, 40);
}

TEST_CASE("parsePathData makes relative commands relative to the current point") {
  const auto path = parsePathData("m10 10 l5 0 h10 v-4 H0 V0");

  REQUIRE(path.countPoints() == 6);
  checkPoint(path.getPoint(1), 15, 10);
  checkPoint(path.getPoint(2), 25, 10);
  checkPoint(path.getPoint(3), 25, 6);
  checkPoint(path.getPoint(4), 0, 6);
  checkPoint(path.getPoint(5), 0, 0);
}

TEST_CASE("parsePathData repeats a move's extra pairs as lines") {
  const auto path = parsePathData("M0 0 10 0 10 10");

  CHECK(verbs(path) == std::vector{Verb::kMove, Verb::kLine, Verb::kLine});
}

TEST_CASE("parsePathData reads packed numbers, signs and exponents") {
  const auto path = parsePathData("M.5-1.5L1e1,2E0-3.5.5");

  REQUIRE(path.countPoints() == 3);
  checkPoint(path.getPoint(0), 0.5, -1.5);
  checkPoint(path.getPoint(1), 10, 2);
  checkPoint(path.getPoint(2), -3.5, 0.5);
}

TEST_CASE("parsePathData reflects the previous control point for a smooth cubic") {
  const auto path = parsePathData("M0 0 C10 0 20 10 30 10 S50 20 60 20");

  REQUIRE(verbs(path) == std::vector{Verb::kMove, Verb::kCubic, Verb::kCubic});
  checkPoint(path.getPoint(4), 40, 10);
  checkPoint(path.getPoint(5), 50, 20);
  checkPoint(path.getPoint(6), 60, 20);
}

TEST_CASE("parsePathData starts a smooth cubic after a line at the current point") {
  const auto path = parsePathData("M0 0 L10 0 S20 10 30 10");

  checkPoint(path.getPoint(2), 10, 0);
}

TEST_CASE("parsePathData keeps quadratic curves and reflects the control point of a smooth one") {
  const auto path = parsePathData("M0 0 Q30 30 60 0 T120 0");

  REQUIRE(verbs(path) == std::vector{Verb::kMove, Verb::kQuad, Verb::kQuad});
  checkPoint(path.getPoint(1), 30, 30);
  checkPoint(path.getPoint(3), 90, -30);
  checkPoint(path.getPoint(4), 120, 0);
}

TEST_CASE("parsePathData draws a half circle arc through its far point") {
  const auto path = parsePathData("M0 0 A10 10 0 0 1 20 0");

  const auto found = verbs(path);
  REQUIRE(found.size() >= 2);
  for (std::size_t i = 1; i < found.size(); ++i) CHECK(found[i] == Verb::kConic);
  checkPoint(lastPoint(path), 20, 0);
  const auto bounds = path.computeTightBounds();
  CHECK(bounds.top() == doctest::Approx(-10));
  CHECK(bounds.bottom() == doctest::Approx(0));
}

TEST_CASE("parsePathData takes the sweep flag as SVG means it") {
  const auto clockwise = parsePathData("M0 0 A10 10 0 0 1 20 0");
  const auto counterClockwise = parsePathData("M0 0 A10 10 0 0 0 20 0");

  CHECK(clockwise.computeTightBounds().top() == doctest::Approx(-10));
  CHECK(counterClockwise.computeTightBounds().bottom() == doctest::Approx(10));
}

TEST_CASE("parsePathData reads packed arc flags") {
  const auto packed = parsePathData("M0 0a10 10 0 0120 0");
  const auto spaced = parsePathData("M0 0a10 10 0 0 1 20 0");

  CHECK(verbs(packed) == verbs(spaced));
  checkPoint(lastPoint(packed), 20, 0);
}

TEST_CASE("parsePathData scales up radii too small to reach the end point") {
  const auto path = parsePathData("M0 0 A1 1 0 0 1 20 0");

  checkPoint(lastPoint(path), 20, 0);
  CHECK(path.computeTightBounds().height() == doctest::Approx(10).epsilon(0.01));
}

TEST_CASE("parsePathData leaves out an arc that ends where it starts") {
  const auto path = parsePathData("M5 5 A10 10 0 0 1 5 5 L9 9");

  CHECK(verbs(path) == std::vector{Verb::kMove, Verb::kLine});
}

TEST_CASE("parsePathData draws an arc without a radius as a line") {
  const auto path = parsePathData("M0 0 A0 10 0 0 1 20 0");

  CHECK(verbs(path) == std::vector{Verb::kMove, Verb::kLine});
}

TEST_CASE("parsePathData starts the next subpath where a closed one started") {
  const auto path = parsePathData("M10 10 L20 10 Z l5 5");

  REQUIRE(verbs(path) == std::vector{Verb::kMove, Verb::kLine, Verb::kClose, Verb::kMove, Verb::kLine});
  checkPoint(path.getPoint(2), 10, 10);
  checkPoint(path.getPoint(3), 15, 15);
}

TEST_CASE("parsePathData keeps the path up to the first error") {
  const auto path = parsePathData("M0 0 L10 10 L20 X 30 30");

  CHECK(verbs(path) == std::vector{Verb::kMove, Verb::kLine});
}

TEST_CASE("parsePathData drops data that does not start with a move") {
  CHECK(parsePathData("L10 10").isEmpty());
  CHECK(parsePathData("Z M0 0 L1 1").isEmpty());
  CHECK(parsePathData("").isEmpty());
}

} // namespace rnwebos::svg
