#include "ViewTransform.h"

#include "../render/RasterTarget.test.h"
#include "ViewTesting.test.h"

#include <doctest/doctest.h>

#include <cmath>

namespace rnwebos {

namespace {

using facebook::react::Transform;
using facebook::react::ViewProps;
using testing::frameAt;

ViewTransform transformOf(const Transform &transform, const facebook::react::LayoutMetrics &metrics) {
  ViewProps props;
  props.transform = transform;
  ViewTransform result;
  result.update(props, metrics);
  return result;
}

SkPoint mapped(const ViewTransform &transform, SkPoint local, facebook::react::Float scale = 1) {
  return transform.mapPoint(local, scale);
}

bool near(SkPoint point, float x, float y) {
  return std::abs(point.x() - x) < 0.01f && std::abs(point.y() - y) < 0.01f;
}

} // namespace

TEST_CASE("a view without a transform maps every point to itself") {
  ViewTransform transform;
  SkPoint local{};

  CHECK(transform.isIdentity());
  CHECK_FALSE(transform.update(ViewProps{}, frameAt(0, 0, 200, 100)));
  CHECK(mapped(transform, {7, 9}) == SkPoint{7, 9});
  CHECK(transform.invertPoint({7, 9}, 1, local));
  CHECK(local == SkPoint{7, 9});
}

TEST_CASE("ViewTransform scales about the box's center by default") {
  const auto scaled = transformOf(Transform::Scale(2, 2, 1), frameAt(100, 50, 200, 100));

  CHECK_FALSE(scaled.isIdentity());
  CHECK(near(mapped(scaled, {100, 50}), 100, 50));
  CHECK(near(mapped(scaled, {0, 0}), -100, -50));
  CHECK(near(mapped(scaled, {200, 100}), 300, 150));
}

TEST_CASE("ViewTransform turns about the transformOrigin when one is set") {
  ViewProps props;
  props.transform = Transform::Scale(2, 2, 1);
  props.transformOrigin.xy = {
      facebook::react::ValueUnit(0, facebook::react::UnitType::Point),
      facebook::react::ValueUnit(0, facebook::react::UnitType::Point)};
  ViewTransform transform;

  CHECK(transform.update(props, frameAt(0, 0, 200, 100)));

  CHECK(near(mapped(transform, {0, 0}), 0, 0));
  CHECK(near(mapped(transform, {10, 10}), 20, 20));
}

TEST_CASE("ViewTransform works in points and maps pixels at the point scale") {
  const auto moved = transformOf(Transform::Translate(10, 5, 0), frameAt(0, 0, 200, 100, 2));
  const auto scaled = transformOf(Transform::Scale(2, 2, 1), frameAt(0, 0, 100, 100, 2));

  CHECK(mapped(moved, {100, 50}, 2) == SkPoint{120, 60});
  CHECK(near(mapped(scaled, {100, 100}, 2), 100, 100));
  CHECK(near(mapped(scaled, {0, 0}, 2), -100, -100));
}

TEST_CASE("ViewTransform moves a plain translation by whole pixels") {
  const auto moved = transformOf(Transform::Translate(10.4, -5.6, 0), frameAt(0, 0, 200, 100));

  CHECK(mapped(moved, {0, 0}) == SkPoint{10, -6});
}

TEST_CASE("A quarter turn about the center sends the top-left corner to the top-right") {
  const auto turned = transformOf(Transform::RotateZ(1.5707963267948966), frameAt(0, 0, 100, 100));

  CHECK(near(mapped(turned, {0, 0}), 100, 0));
}

TEST_CASE("ViewTransform divides by the projected w") {
  ViewProps props;
  props.transform = Transform::Identity();
  props.transform.matrix[15] = 2;
  props.transform.operations.push_back({
      .type = facebook::react::TransformOperationType::Arbitrary,
      .x = facebook::react::ValueUnit(0, facebook::react::UnitType::Point),
      .y = facebook::react::ValueUnit(0, facebook::react::UnitType::Point),
      .z = facebook::react::ValueUnit(0, facebook::react::UnitType::Point),
  });
  props.transformOrigin.xy = {
      facebook::react::ValueUnit(0, facebook::react::UnitType::Point),
      facebook::react::ValueUnit(0, facebook::react::UnitType::Point)};
  ViewTransform transform;
  transform.update(props, frameAt(0, 0, 200, 100));

  CHECK(near(mapped(transform, {40, 30}), 20, 15));
}

TEST_CASE("invertPoint finds the point of the view that shows at a place, and nothing for a flattened view") {
  const auto scaled = transformOf(Transform::Scale(2, 2, 1), frameAt(0, 0, 100, 100));
  const auto flat = transformOf(Transform::Scale(0, 1, 1), frameAt(0, 0, 100, 100));
  SkPoint local{};

  CHECK(scaled.invertPoint({-50, -50}, 1, local));
  CHECK(near(local, 0, 0));
  CHECK(scaled.invertPoint({150, 50}, 1, local));
  CHECK(near(local, 100, 50));
  CHECK_FALSE(flat.invertPoint({10, 10}, 1, local));
}

TEST_CASE("concat draws through the transform, and says when an update changed nothing") {
  ViewProps props;
  props.transform = Transform::Scale(2, 2, 1);
  ViewTransform transform;
  testing::RasterTarget target({.width = 200, .height = 200});
  SkPaint paint;
  paint.setColor(SK_ColorRED);

  CHECK(transform.update(props, frameAt(0, 0, 100, 100)));
  CHECK_FALSE(transform.update(props, frameAt(0, 0, 100, 100)));
  target.canvas().translate(50, 50);
  transform.concat(target.canvas(), 1);
  target.canvas().drawRect(SkRect::MakeWH(100, 100), paint);

  CHECK(target.at(1, 1) == SK_ColorRED);
  CHECK(target.at(199, 199) == SK_ColorRED);
}

} // namespace rnwebos
