#include "MaskedView.h"

#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

using testing::filled;
using testing::frameOf;
using testing::kBlack;
using testing::kRed;
using testing::place;

constexpr int kScreen = 200;

/// A black screen with a masked view on it: a clear mask the size of the view
/// holding an opaque band, and red content.
struct Stage {
  Stage(const facebook::react::LayoutMetrics &box, const facebook::react::LayoutMetrics &bandFrame) {
    const auto fill = frameOf(0, 0, box.frame.size.width, box.frame.size.height);
    place(root, filled(0, 0, 0), frameOf(0, 0, kScreen, kScreen));
    place(masked, std::make_shared<facebook::react::ViewProps>(), box);
    place(mask, std::make_shared<facebook::react::ViewProps>(), fill);
    place(band, filled(0, 0, 0), bandFrame);
    place(content, filled(255, 0, 0), fill);
    mask.mountChild(band, 0);
    root.mountChild(masked, 0);
    root.mountAsRoot();
  }

  std::uint32_t pixel(int x, int y) const {
    return canvas.pixel(x, y);
  }

  const testing::TestCanvas canvas{kScreen};
  Evas *const evas{canvas.evas()};
  ComponentView root{evas, 1};
  MaskedView masked{evas, 2};
  ComponentView mask{evas, 3};
  ComponentView band{evas, 4};
  ComponentView content{evas, 5};
};

} // namespace

TEST_CASE("a masked view shows its content only where the mask is opaque, inside its own box") {
  Stage stage(frameOf(20, 10, 100, 60), frameOf(0, 0, 50, 60));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);

  CHECK(stage.pixel(30, 30) == kRed);
  CHECK(stage.pixel(68, 68) == kRed);
  CHECK(stage.pixel(80, 30) == kBlack);
  CHECK(stage.pixel(10, 30) == kBlack);
  CHECK(stage.pixel(30, 75) == kBlack);
}

TEST_CASE("the mask alone draws nothing") {
  Stage stage(frameOf(20, 10, 100, 60), frameOf(0, 0, 100, 60));
  stage.masked.mountChild(stage.mask, 0);

  CHECK(stage.pixel(60, 40) == kBlack);
}

TEST_CASE("a masked view taller than the screen is masked where it shows") {
  Stage stage(frameOf(0, -300, kScreen, 600), frameOf(0, 350, kScreen, 250));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);

  CHECK(stage.pixel(100, 20) == kBlack);
  CHECK(stage.pixel(100, 60) == kRed);
  CHECK(stage.pixel(100, 190) == kRed);
}

TEST_CASE("a masked view that grows around a mask that stays keeps the mask where it was") {
  Stage stage(frameOf(0, 0, kScreen, 100), frameOf(0, 0, kScreen, 50));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);
  CHECK(stage.pixel(100, 40) == kRed);
  CHECK(stage.pixel(100, 60) == kBlack);

  stage.masked.updateLayout(frameOf(0, 0, kScreen, 160));

  CHECK(stage.pixel(100, 40) == kRed);
  CHECK(stage.pixel(100, 60) == kBlack);
}

TEST_CASE("a mask that moves inside a masked view that scrolls stays where the screen needs it") {
  Stage stage(frameOf(0, 100, kScreen, 400), frameOf(0, 0, kScreen, 400));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);
  CHECK(stage.pixel(100, 150) == kRed);

  stage.masked.updateLayout(frameOf(0, -150, kScreen, 400));
  stage.band.updateLayout(frameOf(0, 200, kScreen, 400));

  CHECK(stage.pixel(100, 30) == kBlack);
  CHECK(stage.pixel(100, 60) == kRed);
  CHECK(stage.pixel(100, 190) == kRed);
}

TEST_CASE("a masked view inside a masked view shows where both masks are opaque") {
  Stage stage(frameOf(0, 0, kScreen, 100), frameOf(0, 0, 150, 100));
  MaskedView inner{stage.evas, 6};
  ComponentView innerMask{stage.evas, 7};
  ComponentView innerBand{stage.evas, 8};
  ComponentView innerContent{stage.evas, 9};
  place(inner, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, kScreen, 100));
  place(innerMask, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, kScreen, 100));
  place(innerBand, filled(0, 0, 0), frameOf(50, 0, 150, 100));
  place(innerContent, filled(255, 0, 0), frameOf(0, 0, kScreen, 100));
  innerMask.mountChild(innerBand, 0);
  inner.mountChild(innerMask, 0);
  inner.mountChild(innerContent, 1);
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(inner, 1);

  CHECK(stage.pixel(25, 50) == kBlack);
  CHECK(stage.pixel(100, 50) == kRed);
  CHECK(stage.pixel(175, 50) == kBlack);
}

TEST_CASE("content mounted behind other content of a masked view is drawn behind it") {
  Stage stage(frameOf(0, 0, kScreen, 100), frameOf(0, 0, kScreen, 100));
  ComponentView behind{stage.evas, 6};
  place(behind, filled(0, 255, 0), frameOf(0, 0, kScreen, 100));
  stage.masked.mountChild(stage.mask, 0);
  stage.masked.mountChild(stage.content, 1);

  stage.masked.mountChild(behind, 1);

  CHECK(stage.pixel(100, 50) == kRed);
}

TEST_CASE("a child mounted ahead of the mask takes its place, and the old mask becomes content") {
  Stage stage(frameOf(0, 0, kScreen, 100), frameOf(0, 0, 100, 100));
  stage.masked.mountChild(stage.content, 0);
  CHECK(stage.pixel(50, 50) == kBlack);

  stage.masked.mountChild(stage.mask, 0);

  CHECK(stage.pixel(50, 50) == kRed);
  CHECK(stage.pixel(150, 50) == kBlack);
}

} // namespace rntizen
