#include "ComponentView.h"

#include "ViewTesting.test.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::colorFromRGBA;
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

TEST_CASE("a root view is a layer of its scene until it is destroyed, and changes ask for a frame") {
  Scene scene(Screen::kSize);
  RasterTarget target(Screen::kSize);
  {
    ComponentView root(scene, 1);
    place(root, background(kRed), frameAt(0, 0, Screen::kSize.width, Screen::kSize.height));
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
