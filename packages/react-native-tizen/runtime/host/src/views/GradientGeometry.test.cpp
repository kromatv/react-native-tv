#include "GradientGeometry.h"

#include <doctest/doctest.h>

#include <array>
#include <cmath>

namespace rntizen {

namespace {

using facebook::react::GradientKeyword;
using facebook::react::LinearGradient;
using facebook::react::RadialGradient;
using facebook::react::RadialGradientShape;
using facebook::react::RadialGradientSize;
using facebook::react::UnitType;
using facebook::react::ValueUnit;

GradientGeometry linear(facebook::react::GradientDirection direction, float width, float height) {
  return {LinearGradient{.direction = direction, .colorStops = {}}, width, height, 1};
}

GradientGeometry radial(RadialGradientShape shape, RadialGradientSize size, float width, float height) {
  RadialGradient gradient;
  gradient.shape = shape;
  gradient.size = size;
  return {gradient, width, height, 1};
}

RadialGradientSize keyword(RadialGradientSize::SizeKeyword size) {
  return {.value = size};
}

} // namespace

TEST_CASE("A 180 degree linear gradient runs down a vertical axis the box's height long") {
  const auto geometry = linear(180.0f, 200, 100);

  CHECK(geometry.axis() == GradientGeometry::Axis::Vertical);
  CHECK(geometry.lineLength() == 100);
  CHECK(geometry.at(0, 0) == doctest::Approx(0));
  CHECK(geometry.at(37, 50) == doctest::Approx(0.5f));
  CHECK(geometry.at(200, 100) == doctest::Approx(1));
}

TEST_CASE("A 90 degree linear gradient runs right along a horizontal axis") {
  const auto geometry = linear(90.0f, 200, 100);

  CHECK(geometry.axis() == GradientGeometry::Axis::Horizontal);
  CHECK(geometry.lineLength() == 200);
  CHECK(geometry.at(50, 10) == doctest::Approx(0.25f));
}

TEST_CASE("A 0 degree linear gradient runs up and a negative angle wraps") {
  CHECK(linear(0.0f, 10, 40).at(5, 40) == doctest::Approx(0));
  CHECK(linear(-90.0f, 40, 10).at(40, 5) == doctest::Approx(0));
  CHECK(linear(-90.0f, 40, 10).axis() == GradientGeometry::Axis::Horizontal);
}

TEST_CASE("A diagonal linear gradient's line spans the box corner to corner as CSS sizes it") {
  const auto geometry = linear(45.0f, 100, 100);
  const auto expected = 100 * std::sin(0.25f * 3.14159265f) * 2;

  CHECK(geometry.axis() == GradientGeometry::Axis::None);
  CHECK(geometry.lineLength() == doctest::Approx(expected));
  CHECK(geometry.at(0, 100) == doctest::Approx(0));
  CHECK(geometry.at(100, 0) == doctest::Approx(1));
}

TEST_CASE("A to-corner keyword points the line so the other two corners share the middle") {
  const auto geometry = linear(GradientKeyword::ToTopRight, 300, 100);

  CHECK(geometry.at(0, 0) == doctest::Approx(0.5f));
  CHECK(geometry.at(300, 100) == doctest::Approx(0.5f));
  CHECK(geometry.at(0, 100) == doctest::Approx(0));
  CHECK(geometry.at(300, 0) == doctest::Approx(1));
}

TEST_CASE("row gives at for each step along a row") {
  const auto geometry = linear(30.0f, 120, 80);
  std::array<float, 12> row{};

  geometry.row(0.5f, 10, 20.5f, static_cast<int>(row.size()), row.data());

  for (std::size_t i = 0; i < row.size(); ++i) {
    CHECK(row[i] == doctest::Approx(geometry.at(0.5f + 10 * static_cast<float>(i), 20.5f)).epsilon(1e-5));
  }
}

TEST_CASE("A farthest-corner circle reaches the corners from the center") {
  const auto geometry = radial(RadialGradientShape::Circle, keyword(RadialGradientSize::SizeKeyword::FarthestCorner), 60, 80);

  CHECK(geometry.lineLength() == doctest::Approx(50));
  CHECK(geometry.at(30, 40) == doctest::Approx(0));
  CHECK(geometry.at(0, 0) == doctest::Approx(1));
}

TEST_CASE("A closest-side ellipse touches the nearer side on each axis") {
  const auto geometry = radial(RadialGradientShape::Ellipse, keyword(RadialGradientSize::SizeKeyword::ClosestSide), 200, 100);

  CHECK(geometry.at(0, 50) == doctest::Approx(1));
  CHECK(geometry.at(100, 0) == doctest::Approx(1));
  CHECK(geometry.at(150, 50) == doctest::Approx(0.5f));
}

TEST_CASE("A farthest-corner ellipse keeps its sides' ratio and passes through the corner") {
  const auto geometry =
      radial(RadialGradientShape::Ellipse, keyword(RadialGradientSize::SizeKeyword::FarthestCorner), 200, 100);

  CHECK(geometry.at(0, 0) == doctest::Approx(1));
  CHECK(geometry.at(200, 100) == doctest::Approx(1));
}

TEST_CASE("Explicit radial dimensions are pixels or percentages of the box") {
  const RadialGradientSize size{
      .value = RadialGradientSize::Dimensions{.x = ValueUnit(50, UnitType::Percent), .y = ValueUnit(10, UnitType::Point)}};
  const auto geometry = radial(RadialGradientShape::Ellipse, size, 100, 100);

  CHECK(geometry.at(100, 50) == doctest::Approx(1));
  CHECK(geometry.at(50, 60) == doctest::Approx(1));
}

TEST_CASE("slope is the most the line fraction changes over one pixel") {
  CHECK(linear(90.0f, 200, 100).slope() == doctest::Approx(1.0f / 200));
}

} // namespace rntizen
