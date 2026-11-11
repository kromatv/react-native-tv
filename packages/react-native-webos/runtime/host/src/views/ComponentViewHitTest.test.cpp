#include "ComponentView.h"

#include "ViewTesting.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

using facebook::react::PointerEventsMode;
using facebook::react::Transform;
using facebook::react::ViewProps;
using testing::frameAt;
using testing::place;
using testing::viewProps;

facebook::react::Props::Shared pointerEvents(PointerEventsMode mode) {
  return viewProps([mode](ViewProps &props) { props.pointerEvents = mode; });
}

facebook::react::Props::Shared plain() {
  return viewProps([](ViewProps &) {});
}

/// A root over the screen, a parent at (100, 100) and its child at (20, 30) inside it.
struct Tree {
  Scene scene{PixelSize{.width = 400, .height = 300}};
  ComponentView root{scene, 1};
  ComponentView parent{scene, 2};
  ComponentView child{scene, 3};

  Tree() {
    place(root, plain(), frameAt(0, 0, 400, 300));
    place(parent, plain(), frameAt(100, 100, 100, 100));
    place(child, plain(), frameAt(20, 30, 40, 40));
    root.mountAsRoot(Scene::kSurfaceOrder);
    root.mountChild(parent, 0);
    parent.mountChild(child, 0);
  }
};

} // namespace

TEST_CASE_FIXTURE(Tree, "hitTest finds the deepest view under a point, and the point in that view's own pixels") {
  SkPoint local{};

  CHECK(root.hitTest({10, 10}, &local) == &root);
  CHECK(local == SkPoint{10, 10});
  CHECK(root.hitTest({105, 105}, &local) == &parent);
  CHECK(local == SkPoint{5, 5});
  CHECK(root.hitTest({125, 135}, &local) == &child);
  CHECK(local == SkPoint{5, 5});
  CHECK(root.hitTest({159, 169}) == &child);
  CHECK(root.hitTest({160, 170}) == &parent);
  CHECK(root.hitTest({400, 10}) == nullptr);
  CHECK(root.hitTest({-1, 10}) == nullptr);
}

TEST_CASE_FIXTURE(Tree, "hitTest prefers the sibling drawn last") {
  ComponentView over(scene, 4);
  place(over, plain(), frameAt(30, 40, 40, 40));
  parent.mountChild(over, 1);

  CHECK(root.hitTest({155, 165}) == &over);
  CHECK(root.hitTest({125, 135}) == &child);
}

TEST_CASE_FIXTURE(Tree, "pointerEvents none hides a view and its subtree from hitTest") {
  parent.updateProps(pointerEvents(PointerEventsMode::None));

  CHECK(root.hitTest({125, 135}) == &root);
  CHECK(root.hitTest({105, 105}) == &root);
}

TEST_CASE_FIXTURE(Tree, "pointerEvents box-none passes the view itself over but not its children") {
  parent.updateProps(pointerEvents(PointerEventsMode::BoxNone));

  CHECK(root.hitTest({125, 135}) == &child);
  CHECK(root.hitTest({105, 105}) == &root);
}

TEST_CASE_FIXTURE(Tree, "pointerEvents box-only takes the points of its children for the view itself") {
  parent.updateProps(pointerEvents(PointerEventsMode::BoxOnly));

  CHECK(root.hitTest({125, 135}) == &parent);
}

TEST_CASE_FIXTURE(Tree, "hitTest finds a child that overflows its parent, unless the parent clips") {
  child.updateLayout(frameAt(80, 80, 40, 40));

  CHECK(root.hitTest({210, 210}) == &child);

  parent.updateProps(viewProps([](ViewProps &props) { props.yogaStyle.setOverflow(facebook::yoga::Overflow::Hidden); }));
  CHECK(root.hitTest({210, 210}) == &root);
  CHECK(root.hitTest({195, 195}) == &child);
}

TEST_CASE_FIXTURE(Tree, "hitTest skips a view that is not displayed") {
  auto hidden = frameAt(100, 100, 100, 100);
  hidden.displayType = facebook::react::DisplayType::None;
  parent.updateLayout(hidden);

  CHECK(root.hitTest({125, 135}) == &root);
}

TEST_CASE_FIXTURE(Tree, "hitTest follows a view's transform back to its own pixels") {
  parent.updateProps(viewProps([](ViewProps &props) { props.transform = Transform::Scale(2, 2, 1); }));
  SkPoint local{};

  CHECK(root.hitTest({60, 60}, &local) == &parent);
  CHECK(local == SkPoint{5, 5});
  CHECK(root.hitTest({100, 120}, &local) == &child);
  CHECK(local == SkPoint{5, 5});
  CHECK(root.hitTest({45, 60}) == &root);

  parent.updateProps(viewProps([](ViewProps &props) { props.transform = Transform::Scale(0, 0, 1); }));
  CHECK(root.hitTest({150, 150}) == &root);
}

TEST_CASE_FIXTURE(Tree, "canvasRect adds up the frames of a view's ancestors, in pixels") {
  CHECK(root.canvasRect() == PixelRect{.x = 0, .y = 0, .width = 400, .height = 300});
  CHECK(parent.canvasRect() == PixelRect{.x = 100, .y = 100, .width = 100, .height = 100});
  CHECK(child.canvasRect() == PixelRect{.x = 120, .y = 130, .width = 40, .height = 40});
  CHECK(child.canvasOrigin().x == 120);
  CHECK(child.canvasOrigin().y == 130);
  CHECK(child.screenRect() == child.canvasRect());
}

TEST_CASE_FIXTURE(Tree, "screenRect is the box around a turned view") {
  parent.updateProps(viewProps([](ViewProps &props) { props.transform = Transform::RotateZ(0.7853981633974483); }));

  const auto rect = parent.screenRect();

  CHECK(rect.x == 79);
  CHECK(rect.y == 79);
  CHECK(rect.width == 142);
  CHECK(rect.height == 142);
}

} // namespace rnwebos
