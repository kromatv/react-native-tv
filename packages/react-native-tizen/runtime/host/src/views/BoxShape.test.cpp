#include "ComponentView.h"

#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

using testing::filled;
using testing::frameOf;
using testing::kBlack;
using testing::kRed;
using testing::place;
using testing::propsWith;

class ClippingView final : public ComponentView {
 public:
  ClippingView(Evas *evas, facebook::react::Tag tag) : ComponentView(evas, tag) {
    setAlwaysClips(true);
  }
};

} // namespace

TEST_CASE("a rounded clip cuts its content to the corners where it is, once moved and once resized") {
  const testing::TestCanvas canvas(200);
  ComponentView root{canvas.evas(), 1};
  ClippingView card{canvas.evas(), 2};
  ComponentView content{canvas.evas(), 3};
  const auto rounded = propsWith([](facebook::react::ViewProps &props) {
    props.borderRadii.all = facebook::react::ValueUnit{20, facebook::react::UnitType::Point};
  });
  place(root, filled(0, 0, 0), frameOf(0, 0, 200, 200));
  place(card, rounded, frameOf(20, 20, 100, 60));
  place(content, filled(255, 0, 0), frameOf(0, 0, 200, 200));
  card.mountChild(content, 0);
  root.mountChild(card, 0);
  root.mountAsRoot();

  CHECK(canvas.pixel(21, 21) == kBlack);
  CHECK(canvas.pixel(70, 21) == kRed);
  CHECK(canvas.pixel(118, 78) == kBlack);
  CHECK(canvas.pixel(70, 50) == kRed);

  card.updateLayout(frameOf(60, 100, 100, 60));

  CHECK(canvas.pixel(70, 50) == kBlack);
  CHECK(canvas.pixel(61, 101) == kBlack);
  CHECK(canvas.pixel(158, 158) == kBlack);
  CHECK(canvas.pixel(110, 101) == kRed);
  CHECK(canvas.pixel(110, 130) == kRed);

  card.updateLayout(frameOf(60, 100, 130, 90));

  CHECK(canvas.pixel(61, 101) == kBlack);
  CHECK(canvas.pixel(188, 188) == kBlack);
  CHECK(canvas.pixel(180, 140) == kRed);
  CHECK(canvas.pixel(120, 188) == kRed);
}

} // namespace rntizen
