#include "ComponentView.h"

#include "ViewFixtures.test.h"

#include <doctest/doctest.h>
#include <react/renderer/graphics/Transform.h>

namespace rntizen {

namespace {

using facebook::react::ViewProps;
using testing::filled;
using testing::frameOf;
using testing::isNear;
using testing::kBlack;
using testing::kRed;
using testing::place;
using testing::propsWith;

facebook::react::Props::Shared scaled(facebook::react::Float factor) {
  return propsWith([factor](ViewProps &props) { props.transform = facebook::react::Transform::Scale(factor, factor, 1); });
}

facebook::react::Props::Shared outlined(facebook::react::Float width) {
  return propsWith([width](ViewProps &props) {
    props.backgroundColor = facebook::react::colorFromRGBA(255, 255, 255, 255);
    props.outlineWidth = width;
    props.outlineColor = facebook::react::colorFromRGBA(255, 0, 0, 255);
  });
}

/// A black screen with a card scaled by a quarter around (100, 100), and a
/// white face that fills the card.
struct ScaledCard {
  explicit ScaledCard(const facebook::react::Props::Shared &faceProps) {
    place(root, filled(0, 0, 0), frameOf(0, 0, 200, 200));
    place(card, scaled(1.25), frameOf(60, 60, 80, 80));
    place(face, faceProps, frameOf(0, 0, 80, 80));
    card.mountChild(face, 0);
    root.mountChild(card, 0);
    root.mountAsRoot();
  }

  const testing::TestCanvas canvas{200};
  ComponentView root{canvas.evas(), 1};
  ComponentView card{canvas.evas(), 2};
  ComponentView face{canvas.evas(), 3};
};

} // namespace

TEST_CASE("a scaled view shows the outline its child draws past the view's own box") {
  const ScaledCard stage(outlined(8));

  CHECK(isNear(stage.canvas.pixel(45, 100), kRed));
  CHECK(isNear(stage.canvas.pixel(100, 45), kRed));
  CHECK(isNear(stage.canvas.pixel(155, 100), kRed));
  CHECK(isNear(stage.canvas.pixel(100, 155), kRed));
  CHECK(stage.canvas.pixel(34, 100) == kBlack);
}

TEST_CASE("a child that gains an outline under a view already scaled shows it whole") {
  ScaledCard stage(outlined(0));
  CHECK(stage.canvas.pixel(45, 100) == kBlack);

  stage.face.updateProps(outlined(8));

  CHECK(isNear(stage.canvas.pixel(45, 100), kRed));
  CHECK(isNear(stage.canvas.pixel(100, 155), kRed));
}

} // namespace rntizen
