#include "TransformMath.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using facebook::react::Transform;

} // namespace

TEST_CASE("isTranslationOnly tells a move from a scale, a rotation or a perspective") {
  CHECK(isTranslationOnly(Transform::Identity()));
  CHECK(isTranslationOnly(Transform::Translate(12, -4, 0)));
  CHECK_FALSE(isTranslationOnly(Transform::Scale(2, 2, 1)));
  CHECK_FALSE(isTranslationOnly(Transform::Rotate(0, 0, 0.5)));
  CHECK_FALSE(isTranslationOnly(Transform::Perspective(800)));
}

TEST_CASE("isTranslationOnly refuses a matrix that divides by a w other than one") {
  auto matrix = Transform::Identity();
  matrix.matrix[15] = 2;

  CHECK_FALSE(isTranslationOnly(matrix));
}

TEST_CASE("aroundCenter keeps the box's center in place and leaves a move as it is") {
  const auto scaled = aroundCenter(Transform::Scale(2, 2, 1), {.width = 200, .height = 100});
  const auto moved = aroundCenter(Transform::Translate(10, 5, 0), {.width = 200, .height = 100});

  CHECK(scaled.matrix[0] == 2);
  CHECK(scaled.matrix[5] == 2);
  CHECK(scaled.matrix[12] == -100);
  CHECK(scaled.matrix[13] == -50);
  CHECK(moved.matrix[12] == 10);
  CHECK(moved.matrix[13] == 5);
  CHECK(isTranslationOnly(moved));
}

} // namespace rnwebos
