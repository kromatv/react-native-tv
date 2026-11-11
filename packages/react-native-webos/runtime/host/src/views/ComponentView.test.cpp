#include "ComponentView.h"

#include "../render/RasterTarget.test.h"
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
using testing::viewProps;

constexpr PixelSize kScreen{.width = 400, .height = 300};

const auto kRed = colorFromRGBA(255, 0, 0, 255);
const auto kBlue = colorFromRGBA(0, 0, 255, 255);

/// A view that records what it is asked to draw, in order.
class RecordingView final : public ComponentView {
 public:
  RecordingView(Scene &scene, facebook::react::Tag tag, std::vector<std::string> &drawn)
      : ComponentView(scene, tag), drawn_(drawn) {}

 protected:
  void drawContent(SkCanvas &, float) override {
    drawn_.push_back("content " + std::to_string(tag()));
  }
  void drawOverChildren(SkCanvas &) override {
    drawn_.push_back("over " + std::to_string(tag()));
  }

 private:
  std::vector<std::string> &drawn_;
};

struct Screen {
  Scene scene{kScreen};
  RasterTarget target{kScreen};
  ComponentView root{scene, 1};

  Screen() {
    place(root, viewProps([](ViewProps &) {}), frameAt(0, 0, kScreen.width, kScreen.height));
    root.mountAsRoot(Scene::kSurfaceOrder);
  }

  void draw() {
    scene.setTransparent(true);
    scene.drawInto(target.canvas());
  }
};

} // namespace

TEST_CASE_FIXTURE(Screen, "a view draws its box at its frame, inside its parent's") {
  ComponentView parent(scene, 2);
  ComponentView child(scene, 3);
  place(parent, background(kRed), frameAt(100, 50, 200, 100));
  place(child, background(kBlue), frameAt(10, 20, 30, 40));
  root.mountChild(parent, 0);
  parent.mountChild(child, 0);

  draw();

  CHECK(target.at(99, 60) == SK_ColorTRANSPARENT);
  CHECK(target.at(100, 50) == SK_ColorRED);
  CHECK(target.at(299, 149) == SK_ColorRED);
  CHECK(target.at(300, 149) == SK_ColorTRANSPARENT);
  CHECK(target.at(110, 70) == SK_ColorBLUE);
  CHECK(target.at(139, 109) == SK_ColorBLUE);
  CHECK(target.at(140, 110) == SK_ColorRED);
}

TEST_CASE_FIXTURE(Screen, "a view places itself in whole pixels at the surface's point scale") {
  ComponentView view(scene, 2);
  place(view, background(kRed), frameAt(10.2, 20.6, 50, 25, 2));
  root.mountChild(view, 0);

  draw();

  CHECK(view.canvasRect() == PixelRect{.x = 20, .y = 41, .width = 100, .height = 50});
  CHECK(target.at(20, 41) == SK_ColorRED);
  CHECK(target.at(19, 41) == SK_ColorTRANSPARENT);
  CHECK(target.at(119, 90) == SK_ColorRED);
  CHECK(target.at(120, 91) == SK_ColorTRANSPARENT);
}

TEST_CASE_FIXTURE(Screen, "siblings draw in their order, the later one on top, wherever they are mounted") {
  ComponentView under(scene, 2);
  ComponentView over(scene, 3);
  place(under, background(kRed), frameAt(0, 0, 100, 100));
  place(over, background(kBlue), frameAt(50, 0, 100, 100));
  root.mountChild(over, 0);
  root.mountChild(under, 0);

  draw();

  CHECK(root.children().size() == 2);
  CHECK(root.children()[0] == &under);
  CHECK(target.at(25, 50) == SK_ColorRED);
  CHECK(target.at(75, 50) == SK_ColorBLUE);

  root.unmountChild(over);
  draw();
  CHECK(over.parent() == nullptr);
  CHECK(target.at(75, 50) == SK_ColorRED);
  CHECK(target.at(125, 50) == SK_ColorTRANSPARENT);
}

TEST_CASE_FIXTURE(Screen, "a view draws its content, then its children, then what goes over them") {
  std::vector<std::string> drawn;
  RecordingView parent(scene, 2, drawn);
  RecordingView child(scene, 3, drawn);
  place(parent, background(kRed), frameAt(0, 0, 100, 100));
  place(child, background(kBlue), frameAt(0, 0, 50, 50));
  root.mountChild(parent, 0);
  parent.mountChild(child, 0);

  draw();

  CHECK(drawn == std::vector<std::string>{"content 2", "content 3", "over 3", "over 2"});
}

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

TEST_CASE_FIXTURE(Screen, "a view with no opacity, or no display, draws nothing of its subtree") {
  std::vector<std::string> drawn;
  RecordingView invisible(scene, 2, drawn);
  RecordingView hidden(scene, 3, drawn);
  RecordingView child(scene, 4, drawn);
  place(invisible, viewProps([](ViewProps &props) { props.opacity = 0; }), frameAt(0, 0, 100, 100));
  auto none = frameAt(0, 0, 100, 100);
  none.displayType = facebook::react::DisplayType::None;
  place(hidden, background(kRed), none);
  place(child, background(kBlue), frameAt(0, 0, 50, 50));
  root.mountChild(invisible, 0);
  root.mountChild(hidden, 1);
  hidden.mountChild(child, 0);

  draw();

  CHECK(drawn.empty());
  CHECK(target.at(25, 25) == SK_ColorTRANSPARENT);
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

TEST_CASE("a root view is a layer of its scene until it is destroyed, and changes ask for a frame") {
  Scene scene(kScreen);
  RasterTarget target(kScreen);
  {
    ComponentView root(scene, 1);
    place(root, background(kRed), frameAt(0, 0, kScreen.width, kScreen.height));
    root.mountAsRoot(Scene::kSurfaceOrder);
    scene.drawInto(target.canvas());
    CHECK(target.at(10, 10) == SK_ColorRED);

    scene.render();
    CHECK_FALSE(scene.isInvalid());
    root.updateProps(background(kBlue));
    CHECK(scene.isInvalid());
    scene.render();
    root.updateLayout(frameAt(0, 0, 10, 10));
    CHECK(scene.isInvalid());
    scene.render();
  }

  CHECK(scene.isInvalid());
  scene.drawInto(target.canvas());
  CHECK(target.at(10, 10) == SK_ColorBLACK);
}

} // namespace rnwebos
