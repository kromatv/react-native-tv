#include "GradientGeometry.h"

#include <doctest/doctest.h>

#include <array>
#include <cmath>

namespace rnwebos {

namespace {

using facebook::react::GradientKeyword;
using facebook::react::LinearGradient;
using facebook::react::RadialGradient;
using facebook::react::RadialGradientShape;
using facebook::react::RadialGradientSize;
using facebook::react::UnitType;
using facebook::react::ValueUnit;

GradientGeometry linear(facebook::react::GradientDirection direction, float width, float height) {
  return layOutGradient(LinearGradient{.direction = direction, .colorStops = {}}, width, height, 1);
}

GradientGeometry radial(RadialGradientShape shape, RadialGradientSize size, float width, float height) {
  RadialGradient gradient;
  gradient.shape = shape;
  gradient.size = size;
  return layOutGradient(gradient, width, height, 1);
}

RadialGradientSize keyword(RadialGradientSize::SizeKeyword size) {
  return {.value = size};
}

void checkPoint(SkPoint actual, SkPoint expected) {
  CHECK(actual.x() == doctest::Approx(expected.x()).epsilon(1e-4));
  CHECK(actual.y() == doctest::Approx(expected.y()).epsilon(1e-4));
}

// The fraction of the gradient line the box point (`x`, `y`) falls on.
float fractionAt(const GradientGeometry &geometry, float x, float y) {
  if (geometry.radial) {
    return std::hypot((x - geometry.center.x()) / geometry.radiusX, (y - geometry.center.y()) / geometry.radiusY);
  }
  const auto line = geometry.end - geometry.start;
  return SkPoint::DotProduct(SkPoint{x, y} - geometry.start, line) / line.dot(line);
}

} // namespace

TEST_CASE("A 180 degree linear gradient runs down the middle, the box's height long") {
  const auto geometry = linear(180.0f, 200, 100);

  CHECK_FALSE(geometry.radial);
  CHECK(geometry.lineLength == 100);
  checkPoint(geometry.start, {100, 0});
  checkPoint(geometry.end, {100, 100});
}

TEST_CASE("A 90 degree linear gradient runs right, the box's width long") {
  const auto geometry = linear(90.0f, 200, 100);

  CHECK(geometry.lineLength == 200);
  checkPoint(geometry.start, {0, 50});
  checkPoint(geometry.end, {200, 50});
}

TEST_CASE("A 0 degree linear gradient runs up and a negative angle wraps") {
  checkPoint(linear(0.0f, 10, 40).start, {5, 40});
  checkPoint(linear(0.0f, 10, 40).end, {5, 0});
  checkPoint(linear(-90.0f, 40, 10).start, {40, 5});
  checkPoint(linear(-90.0f, 40, 10).end, {0, 5});
}

TEST_CASE("A diagonal linear gradient's line spans the box corner to corner as CSS sizes it") {
  const auto geometry = linear(45.0f, 100, 100);
  const auto expected = 100 * std::sin(0.25f * 3.14159265f) * 2;

  CHECK(geometry.lineLength == doctest::Approx(expected));
  checkPoint(geometry.start, {0, 100});
  checkPoint(geometry.end, {100, 0});
}

TEST_CASE("A diagonal line in a box that is not square ends where the corners project on it") {
  const auto geometry = linear(30.0f, 120, 80);

  CHECK(fractionAt(geometry, 0, 80) == doctest::Approx(0));
  CHECK(fractionAt(geometry, 120, 0) == doctest::Approx(1));
  CHECK(fractionAt(geometry, 60, 40) == doctest::Approx(0.5f));
}

TEST_CASE("A to-corner keyword points the line so the other two corners share the middle") {
  const auto geometry = linear(GradientKeyword::ToTopRight, 300, 100);

  CHECK(fractionAt(geometry, 0, 0) == doctest::Approx(0.5f));
  CHECK(fractionAt(geometry, 300, 100) == doctest::Approx(0.5f));
  CHECK(fractionAt(geometry, 0, 100) == doctest::Approx(0));
  CHECK(fractionAt(geometry, 300, 0) == doctest::Approx(1));
}

TEST_CASE("Each to-corner keyword ends the line at its corner") {
  CHECK(fractionAt(linear(GradientKeyword::ToBottomRight, 300, 100), 300, 100) == doctest::Approx(1));
  CHECK(fractionAt(linear(GradientKeyword::ToBottomLeft, 300, 100), 0, 100) == doctest::Approx(1));
  CHECK(fractionAt(linear(GradientKeyword::ToTopLeft, 300, 100), 0, 0) == doctest::Approx(1));
}

TEST_CASE("A farthest-corner circle reaches the corners from the center") {
  const auto geometry = radial(RadialGradientShape::Circle, keyword(RadialGradientSize::SizeKeyword::FarthestCorner), 60, 80);

  CHECK(geometry.radial);
  CHECK(geometry.lineLength == doctest::Approx(50));
  checkPoint(geometry.center, {30, 40});
  CHECK(geometry.radiusX == doctest::Approx(50));
  CHECK(geometry.radiusY == doctest::Approx(50));
}

TEST_CASE("A closest-side ellipse touches the nearer side on each axis") {
  const auto geometry = radial(RadialGradientShape::Ellipse, keyword(RadialGradientSize::SizeKeyword::ClosestSide), 200, 100);

  CHECK(geometry.radiusX == doctest::Approx(100));
  CHECK(geometry.radiusY == doctest::Approx(50));
}

TEST_CASE("A closest-side circle takes the smaller of the two distances") {
  const auto geometry = radial(RadialGradientShape::Circle, keyword(RadialGradientSize::SizeKeyword::ClosestSide), 200, 100);

  CHECK(geometry.radiusX == doctest::Approx(50));
  CHECK(geometry.radiusY == doctest::Approx(50));
}

TEST_CASE("A farthest-corner ellipse keeps its sides' ratio and passes through the corner") {
  const auto geometry =
      radial(RadialGradientShape::Ellipse, keyword(RadialGradientSize::SizeKeyword::FarthestCorner), 200, 100);

  CHECK(geometry.radiusX / geometry.radiusY == doctest::Approx(2));
  CHECK(fractionAt(geometry, 0, 0) == doctest::Approx(1));
  CHECK(fractionAt(geometry, 200, 100) == doctest::Approx(1));
}

TEST_CASE("Explicit radial dimensions are pixels or percentages of the box") {
  const RadialGradientSize size{
      .value = RadialGradientSize::Dimensions{.x = ValueUnit(50, UnitType::Percent), .y = ValueUnit(10, UnitType::Point)}};
  const auto geometry = radial(RadialGradientShape::Ellipse, size, 100, 100);

  CHECK(geometry.radiusX == doctest::Approx(50));
  CHECK(geometry.radiusY == doctest::Approx(10));
}

TEST_CASE("A radial gradient's position moves its center, a length in points at the point scale") {
  RadialGradient gradient;
  gradient.shape = RadialGradientShape::Circle;
  gradient.size = keyword(RadialGradientSize::SizeKeyword::ClosestSide);
  gradient.position.left = ValueUnit(25, UnitType::Percent);
  gradient.position.bottom = ValueUnit(10, UnitType::Point);

  const auto geometry = layOutGradient(gradient, 200, 100, 2);

  checkPoint(geometry.center, {50, 80});
  CHECK(geometry.radiusX == doctest::Approx(20));
}

} // namespace rnwebos
