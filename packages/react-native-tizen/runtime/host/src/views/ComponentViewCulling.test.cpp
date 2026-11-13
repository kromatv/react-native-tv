#include "ComponentView.h"

#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

using facebook::react::Float;
using testing::filled;
using testing::frameOf;
using testing::kBlack;
using testing::kRed;
using testing::place;

constexpr std::uint32_t kGreen = 0xff00ff00;

/// A view that shifts its children as a ScrollView does and lets none show
/// outside its box, though it clips nothing: a child drawn outside the box
/// was not hidden.
class Scroller final : public ComponentView {
 public:
  using ComponentView::ComponentView;

  void scrollTo(Float y) {
    offset_ = y;
    placeChildren();
  }

 private:
  facebook::react::Point childrenOffset() const override {
    return {.x = 0, .y = -offset_};
  }

  std::optional<PixelRect> visibleRegionForChildren(const std::optional<PixelRect> &) const override {
    return canvasRect();
  }

  Float offset_{0};
};

/// A black screen with a scroller over its top half: a red row at the top, a
/// holder three rows tall, and inside it a red row below the fold with a green
/// square.
struct Page {
  Page() {
    place(root, filled(0, 0, 0), frameOf(0, 0, 200, 200));
    place(scroller, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, 200, 100));
    place(first, filled(255, 0, 0), frameOf(0, 0, 200, 40));
    place(holder, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, 200, 300));
    place(second, filled(255, 0, 0), frameOf(0, 150, 200, 40));
    place(square, filled(0, 255, 0), frameOf(10, 10, 20, 20));
    second.mountChild(square, 0);
    holder.mountChild(second, 0);
    scroller.mountChild(first, 0);
    scroller.mountChild(holder, 1);
    root.mountChild(scroller, 0);
    root.mountAsRoot();
  }

  const testing::TestCanvas canvas{200};
  ComponentView root{canvas.evas(), 1};
  Scroller scroller{canvas.evas(), 2};
  ComponentView first{canvas.evas(), 3};
  ComponentView holder{canvas.evas(), 4};
  ComponentView second{canvas.evas(), 5};
  ComponentView square{canvas.evas(), 6};
};

} // namespace

TEST_CASE("a view outside what its scroller shows is hidden, and shows with what it holds once scrolled in") {
  Page page;
  CHECK(page.canvas.pixel(100, 20) == kRed);
  CHECK(page.canvas.pixel(100, 160) == kBlack);

  page.scroller.scrollTo(100);

  CHECK(page.canvas.pixel(100, 20) == kBlack);
  CHECK(page.canvas.pixel(100, 60) == kRed);
  CHECK(page.canvas.pixel(20, 70) == kGreen);

  page.scroller.scrollTo(0);

  CHECK(page.canvas.pixel(100, 20) == kRed);
  CHECK(page.canvas.pixel(100, 60) == kBlack);
  CHECK(page.canvas.pixel(100, 160) == kBlack);
}

TEST_CASE("what changed inside a hidden view is where it belongs when the view returns") {
  Page page;
  page.square.updateLayout(frameOf(150, 10, 20, 20));
  page.scroller.scrollTo(20);
  CHECK(page.canvas.pixel(100, 140) == kBlack);

  page.scroller.scrollTo(100);

  CHECK(page.canvas.pixel(160, 70) == kGreen);
  CHECK(page.canvas.pixel(20, 70) == kRed);
}

TEST_CASE("a view hidden by a short scroller shows once the scroller grows, though nothing moved") {
  Page page;
  CHECK(page.canvas.pixel(100, 160) == kBlack);

  page.scroller.updateLayout(frameOf(0, 0, 200, 200));

  CHECK(page.canvas.pixel(100, 160) == kRed);
  CHECK(page.canvas.pixel(20, 170) == kGreen);
}

TEST_CASE("a view that clips hides what lies far outside its box, and one that does not hides nothing") {
  const testing::TestCanvas canvas(200);
  ComponentView root{canvas.evas(), 1};
  ComponentView open{canvas.evas(), 2};
  ComponentView far{canvas.evas(), 3};
  place(root, filled(0, 0, 0), frameOf(0, 0, 1000, 1000));
  place(open, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, 10, 10));
  place(far, filled(255, 0, 0), frameOf(180, 180, 20, 20));
  open.mountChild(far, 0);
  root.mountChild(open, 0);
  root.mountAsRoot();
  CHECK(canvas.pixel(190, 190) == kRed);

  open.updateProps(testing::propsWith([](facebook::react::ViewProps &props) {
    props.yogaStyle.setOverflow(facebook::yoga::Overflow::Hidden);
  }));
  CHECK(canvas.pixel(190, 190) == kBlack);

  far.updateLayout(frameOf(400, 400, 20, 20));
  far.updateLayout(frameOf(2, 2, 6, 6));
  CHECK(canvas.pixel(4, 4) == kRed);
}

} // namespace rntizen
