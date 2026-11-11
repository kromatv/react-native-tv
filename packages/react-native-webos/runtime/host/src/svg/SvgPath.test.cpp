#include "SvgPath.h"

#include "include/core/SkRRect.h"

#include <doctest/doctest.h>

namespace rnwebos::svg {

TEST_CASE("an ellipse starts at its rightmost point and winds clockwise") {
  const auto path = ellipsePath({50, 40}, 30, 20);

  CHECK(path.getPoint(0).x() == doctest::Approx(80));
  CHECK(path.getPoint(0).y() == doctest::Approx(40));
  CHECK(path.getPoint(1).y() > 40);
  CHECK(path.getBounds() == SkRect::MakeLTRB(20, 20, 80, 60));
}

TEST_CASE("an ellipse without both radii is empty") {
  CHECK(ellipsePath({0, 0}, 0, 10).isEmpty());
  CHECK(ellipsePath({0, 0}, 10, -1).isEmpty());
}

TEST_CASE("a rectangle without size is empty") {
  CHECK(rectPath({0, 0}, 0, 10, {}, {}).isEmpty());
  CHECK(rectPath({0, 0}, 10, -2, {}, {}).isEmpty());
}

TEST_CASE("a rectangle without radii is a plain rectangle from its top-left corner") {
  const auto path = rectPath({10, 20}, 30, 40, {}, {});

  SkRect rect;
  REQUIRE(path.isRect(&rect));
  CHECK(rect == SkRect::MakeXYWH(10, 20, 30, 40));
  CHECK(path.getPoint(0) == SkPoint{10, 20});
  CHECK(path.getPoint(1) == SkPoint{40, 20});
}

TEST_CASE("a rectangle's missing radius takes the other's value") {
  SkRRect rounded;
  REQUIRE(rectPath({0, 0}, 100, 60, 10, {}).isRRect(&rounded));

  CHECK(rounded.getSimpleRadii() == SkVector{10, 10});
}

TEST_CASE("a rectangle's radii are clamped to half the side each rounds") {
  SkRRect rounded;
  REQUIRE(rectPath({0, 0}, 100, 60, 80, 10).isRRect(&rounded));
  CHECK(rounded.getSimpleRadii() == SkVector{50, 10});

  REQUIRE(rectPath({0, 0}, 100, 60, 10, 40).isRRect(&rounded));
  CHECK(rounded.getSimpleRadii() == SkVector{10, 30});
}

TEST_CASE("a rounded rectangle starts on its top edge, after the corner") {
  const auto path = rectPath({0, 0}, 100, 60, 10, 10);

  CHECK(path.getPoint(0) == SkPoint{10, 0});
}

} // namespace rnwebos::svg
