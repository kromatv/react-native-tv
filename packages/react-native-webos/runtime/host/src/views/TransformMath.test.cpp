#include "TransformMath.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

using facebook::react::Transform;

constexpr PixelRect kBox{.x = 100, .y = 50, .width = 200, .height = 100};

} // namespace

TEST_CASE("isTranslationOnly tells a move from a scale, a rotation or a perspective") {
  CHECK(isTranslationOnly(Transform::Identity()));
  CHECK(isTranslationOnly(Transform::Translate(12, -4, 0)));
  CHECK_FALSE(isTranslationOnly(Transform::Scale(2, 2, 1)));
  CHECK_FALSE(isTranslationOnly(Transform::Rotate(0, 0, 0.5)));
  CHECK_FALSE(isTranslationOnly(Transform::Perspective(800)));
}

TEST_CASE("transformedPoint scales about the box's top-left corner by default") {
  const auto point = transformedPoint(Transform::Scale(2, 2, 1), {.x = 110, .y = 60}, kBox, 1);

  CHECK(point == PixelPoint{.x = 120, .y = 70});
}

TEST_CASE("aroundCenter keeps the box's center in place") {
  const auto scaled = aroundCenter(Transform::Scale(2, 2, 1), {.width = 200, .height = 100});

  CHECK(transformedPoint(scaled, {.x = 200, .y = 100}, kBox, 1) == PixelPoint{.x = 200, .y = 100});
  CHECK(transformedPoint(scaled, {.x = 100, .y = 50}, kBox, 1) == PixelPoint{.x = 0, .y = 0});
}

TEST_CASE("transformedPoint works in points and returns pixels at the point scale") {
  const auto shifted = transformedPoint(Transform::Translate(10, 5, 0), {.x = 100, .y = 50}, kBox, 2);

  CHECK(shifted == PixelPoint{.x = 120, .y = 60});
}

TEST_CASE("A quarter turn about the center sends the top-left corner to the top-right") {
  const auto turned = aroundCenter(Transform::RotateZ(1.5707963267948966), {.width = 100, .height = 100});
  const PixelRect box{.x = 0, .y = 0, .width = 100, .height = 100};

  CHECK(transformedPoint(turned, {.x = 0, .y = 0}, box, 1) == PixelPoint{.x = 100, .y = 0});
}

TEST_CASE("transformedPoint divides by the projected w") {
  auto matrix = Transform::Identity();
  matrix.matrix[15] = 2;

  CHECK(transformedPoint(matrix, {.x = 140, .y = 80}, kBox, 1) == PixelPoint{.x = 120, .y = 65});
}

} // namespace rntizen
