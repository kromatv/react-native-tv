#include "ComponentView.h"

#include "ViewTesting.test.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::colorFromRGBA;
using facebook::react::Transform;
using facebook::react::ViewProps;
using testing::background;
using testing::frameAt;
using testing::place;
using testing::RasterTarget;
using testing::RecordingView;
using testing::Screen;
using testing::viewProps;

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);

} // namespace

TEST_CASE_FIXTURE(Screen, "the opacity of a view with nothing else to blend goes into its paint") {
  ComponentView view(scene, 2);
  place(
      view,
      viewProps([](ViewProps &props) {
        props.backgroundColor = kRed;
        props.opacity = 0.5;
      }),
      frameAt(0, 0, 100, 100));
  root.mountChild(view, 0);

  draw();

  CHECK(SkColorGetA(target.at(50, 50)) == doctest::Approx(128).epsilon(0.02));
  CHECK(SkColorGetR(target.at(50, 50)) == 255);
}

TEST_CASE_FIXTURE(Screen, "the opacity of a view with children fades the whole subtree as one picture") {
  ComponentView parent(scene, 2);
  ComponentView child(scene, 3);
  place(
      parent,
      viewProps([](ViewProps &props) {
        props.backgroundColor = kRed;
        props.opacity = 0.5;
      }),
      frameAt(0, 0, 100, 100));
  place(child, background(kBlue), frameAt(0, 0, 50, 50));
  root.mountChild(parent, 0);
  parent.mountChild(child, 0);

  draw();

  const auto overlap = target.at(25, 25);
  CHECK(SkColorGetA(overlap) == doctest::Approx(128).epsilon(0.02));
  CHECK(SkColorGetR(overlap) == 0);
  CHECK(SkColorGetB(overlap) == 255);
  CHECK(SkColorGetR(target.at(75, 75)) == 255);
}

TEST_CASE_FIXTURE(Screen, "overflow hidden clips the children to the box, and visible lets them out") {
  ComponentView parent(scene, 2);
  ComponentView child(scene, 3);
  place(parent, viewProps([](ViewProps &) {}), frameAt(100, 100, 100, 100));
  place(child, background(kBlue), frameAt(50, 50, 100, 100));
  root.mountChild(parent, 0);
  parent.mountChild(child, 0);

  draw();
  CHECK(target.at(175, 175) == SK_ColorBLUE);
  CHECK(target.at(225, 225) == SK_ColorBLUE);

  parent.updateProps(viewProps([](ViewProps &props) { props.yogaStyle.setOverflow(facebook::yoga::Overflow::Hidden); }));
  draw();
  CHECK(target.at(175, 175) == SK_ColorBLUE);
  CHECK(target.at(225, 225) == SK_ColorTRANSPARENT);
}

TEST_CASE_FIXTURE(Screen, "a translation moves the view by whole pixels") {
  ComponentView view(scene, 2);
  place(
      view,
      viewProps([](ViewProps &props) {
        props.backgroundColor = kRed;
        props.transform = Transform::Translate(20.4, -10, 0);
      }),
      frameAt(100, 100, 50, 50));
  root.mountChild(view, 0);

  draw();

  CHECK(target.at(119, 100) == SK_ColorTRANSPARENT);
  CHECK(target.at(120, 90) == SK_ColorRED);
  CHECK(target.at(169, 139) == SK_ColorRED);
  CHECK(target.at(170, 139) == SK_ColorTRANSPARENT);
  CHECK(view.screenRect() == PixelRect{.x = 120, .y = 90, .width = 50, .height = 50});
  CHECK(view.canvasRect() == PixelRect{.x = 100, .y = 100, .width = 50, .height = 50});
}

TEST_CASE_FIXTURE(Screen, "a scale grows the view and its children about the view's center") {
  ComponentView parent(scene, 2);
  ComponentView child(scene, 3);
  place(
      parent,
      viewProps([](ViewProps &props) {
        props.backgroundColor = kRed;
        props.transform = Transform::Scale(2, 2, 1);
      }),
      frameAt(150, 100, 100, 100));
  place(child, background(kBlue), frameAt(0, 0, 10, 10));
  root.mountChild(parent, 0);
  parent.mountChild(child, 0);

  draw();

  CHECK(target.at(99, 150) == SK_ColorTRANSPARENT);
  CHECK(target.at(101, 51) == SK_ColorBLUE);
  CHECK(target.at(118, 68) == SK_ColorBLUE);
  CHECK(target.at(122, 72) == SK_ColorRED);
  CHECK(target.at(298, 248) == SK_ColorRED);
  CHECK(target.at(301, 150) == SK_ColorTRANSPARENT);
  CHECK(parent.screenRect() == PixelRect{.x = 100, .y = 50, .width = 200, .height = 200});
  CHECK(child.screenRect() == PixelRect{.x = 100, .y = 50, .width = 20, .height = 20});
}

TEST_CASE_FIXTURE(Screen, "a view off the screen is skipped, its subtree with it") {
  std::vector<std::string> drawn;
  RecordingView onScreen(scene, 2, drawn);
  RecordingView below(scene, 3, drawn);
  RecordingView belowChild(scene, 4, drawn);
  RecordingView movedIn(scene, 5, drawn);
  place(onScreen, background(kRed), frameAt(0, 0, 50, 50));
  place(below, background(kRed), frameAt(0, 2000, 50, 50));
  place(belowChild, background(kRed), frameAt(0, 0, 10, 10));
  place(
      movedIn,
      viewProps([](ViewProps &props) {
        props.transform = Transform::Translate(0, -1900, 0);
      }),
      frameAt(60, 2000, 50, 50));
  root.mountChild(onScreen, 0);
  root.mountChild(below, 1);
  below.mountChild(belowChild, 0);
  root.mountChild(movedIn, 2);

  draw();

  CHECK(drawn == std::vector<std::string>{"content 2", "over 2", "content 5", "over 5"});
}

TEST_CASE_FIXTURE(Screen, "a view just off the screen still draws children that reach back in") {
  ComponentView parent(scene, 2);
  ComponentView child(scene, 3);
  place(parent, viewProps([](ViewProps &) {}), frameAt(0, 320, 100, 20));
  place(child, background(kBlue), frameAt(0, -60, 100, 60));
  root.mountChild(parent, 0);
  parent.mountChild(child, 0);

  draw();

  CHECK(target.at(50, 280) == SK_ColorBLUE);
}

} // namespace rnwebos
