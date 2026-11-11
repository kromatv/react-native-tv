#include "PathData.h"

#include <doctest/doctest.h>

#include <cmath>
#include <vector>

namespace rntizen::svg {

namespace {

using Kind = PathCommand::Kind;

void checkPoint(const Point &actual, double x, double y) {
  CHECK(actual.x == doctest::Approx(x));
  CHECK(actual.y == doctest::Approx(y));
}

std::vector<Kind> kinds(const Path &path) {
  std::vector<Kind> found;
  for (const auto &command : path.commands()) found.push_back(command.kind);
  return found;
}

} // namespace

TEST_CASE("parsePathData reads absolute moves, lines and a close") {
  const auto path = parsePathData("M10 20 L30 40 Z");

  REQUIRE(kinds(path) == std::vector{Kind::Move, Kind::Line, Kind::Close});
  checkPoint(path.commands()[1].to, 30, 40);
  checkPoint(path.current(), 10, 20);
}

TEST_CASE("parsePathData makes relative commands relative to the current point") {
  const auto path = parsePathData("m10 10 l5 0 h10 v-4 H0 V0");

  const auto &commands = path.commands();
  REQUIRE(commands.size() == 6);
  checkPoint(commands[1].to, 15, 10);
  checkPoint(commands[2].to, 25, 10);
  checkPoint(commands[3].to, 25, 6);
  checkPoint(commands[4].to, 0, 6);
  checkPoint(commands[5].to, 0, 0);
}

TEST_CASE("parsePathData repeats a move's extra pairs as lines") {
  const auto path = parsePathData("M0 0 10 0 10 10");

  CHECK(kinds(path) == std::vector{Kind::Move, Kind::Line, Kind::Line});
}

TEST_CASE("parsePathData reads packed numbers, signs and exponents") {
  const auto path = parsePathData("M.5-1.5L1e1,2E0-3.5.5");

  REQUIRE(path.commands().size() == 3);
  checkPoint(path.commands()[0].to, 0.5, -1.5);
  checkPoint(path.commands()[1].to, 10, 2);
  checkPoint(path.commands()[2].to, -3.5, 0.5);
}

TEST_CASE("parsePathData reflects the previous control point for a smooth cubic") {
  const auto path = parsePathData("M0 0 C10 0 20 10 30 10 S50 20 60 20");

  const auto &smooth = path.commands()[2];
  REQUIRE(smooth.kind == Kind::Cubic);
  checkPoint(smooth.control1, 40, 10);
  checkPoint(smooth.control2, 50, 20);
  checkPoint(smooth.to, 60, 20);
}

TEST_CASE("parsePathData starts a smooth cubic after a line at the current point") {
  const auto path = parsePathData("M0 0 L10 0 S20 10 30 10");

  checkPoint(path.commands()[2].control1, 10, 0);
}

TEST_CASE("parsePathData turns quadratic curves into the equivalent cubic") {
  const auto path = parsePathData("M0 0 Q30 30 60 0 T120 0");

  const auto &first = path.commands()[1];
  checkPoint(first.control1, 20, 20);
  checkPoint(first.control2, 40, 20);
  const auto &reflected = path.commands()[2];
  checkPoint(reflected.control1, 80, -20);
  checkPoint(reflected.to, 120, 0);
}

TEST_CASE("parsePathData draws a half circle arc as cubics through its far point") {
  const auto path = parsePathData("M0 0 A10 10 0 0 1 20 0");

  const auto &commands = path.commands();
  REQUIRE(commands.size() >= 3);
  for (std::size_t i = 1; i < commands.size(); ++i) CHECK(commands[i].kind == Kind::Cubic);
  checkPoint(commands.back().to, 20, 0);
  bool reachesTop = false;
  for (const auto &command : commands) reachesTop = reachesTop || std::abs(command.to.y + 10) < 1e-6;
  CHECK(reachesTop);
}

TEST_CASE("parsePathData reads packed arc flags") {
  const auto packed = parsePathData("M0 0a10 10 0 0120 0");
  const auto spaced = parsePathData("M0 0a10 10 0 0 1 20 0");

  REQUIRE(packed.commands().size() == spaced.commands().size());
  checkPoint(packed.current(), 20, 0);
}

TEST_CASE("parsePathData scales up radii too small to reach the end point") {
  const auto path = parsePathData("M0 0 A1 1 0 0 1 20 0");

  checkPoint(path.current(), 20, 0);
  CHECK(path.bounds().height == doctest::Approx(10).epsilon(0.01));
}

TEST_CASE("parsePathData keeps the path up to the first error") {
  const auto path = parsePathData("M0 0 L10 10 L20 X 30 30");

  CHECK(kinds(path) == std::vector{Kind::Move, Kind::Line});
}

TEST_CASE("parsePathData drops data that does not start with a move") {
  CHECK(parsePathData("L10 10").empty());
  CHECK(parsePathData("").empty());
}

} // namespace rntizen::svg
