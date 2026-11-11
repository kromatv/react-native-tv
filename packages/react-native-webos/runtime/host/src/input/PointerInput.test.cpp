#include "PointerInput.h"

#include "../views/ViewTesting.test.h"

#include <doctest/doctest.h>

#include <optional>

namespace rnwebos {

namespace {

using testing::frameAt;
using testing::place;
using testing::viewProps;

class ScrollingView final : public ComponentView {
 public:
  using ComponentView::ComponentView;

  bool scrollByWheel(facebook::react::Point delta) override {
    scrolled = delta;
    return takesWheel;
  }

  std::optional<facebook::react::Point> scrolled;
  bool takesWheel{true};
};

class CoveringLayer final : public SceneLayer {
 public:
  void draw(SkCanvas &) override {}
  bool blocksPointer() const override {
    return true;
  }
};

facebook::react::Props::Shared plain() {
  return viewProps([](facebook::react::ViewProps &) {});
}

/// A root over the screen, a scrolling list at (50, 50) and a row inside it.
struct Tree {
  Scene scene{PixelSize{.width = 400, .height = 300}};
  ComponentView root{scene, 1};
  ScrollingView list{scene, 2};
  ComponentView row{scene, 3};
  PointerInput pointer{scene, [this](facebook::react::Tag tag) -> ComponentView * {
                         if (tag == 1) return &root;
                         if (tag == 2) return &list;
                         return tag == 3 ? &row : nullptr;
                       }};

  Tree() {
    place(root, plain(), frameAt(0, 0, 400, 300));
    place(list, plain(), frameAt(50, 50, 200, 200));
    place(row, plain(), frameAt(0, 0, 200, 40));
    root.mountAsRoot(Scene::kSurfaceOrder);
    root.mountChild(list, 0);
    list.mountChild(row, 0);
  }

  SDL_Event wheelAt(float x, float y, float notches) const {
    SDL_Event event{};
    event.type = SDL_EVENT_MOUSE_WHEEL;
    event.wheel.y = notches;
    event.wheel.mouse_x = x;
    event.wheel.mouse_y = y;
    return event;
  }
};

} // namespace

TEST_CASE_FIXTURE(Tree, "the scene finds the view under the pointer, top layer first") {
  SkPoint local{};

  CHECK(scene.viewAt({60, 60}, &local) == &row);
  CHECK(local == SkPoint{10, 10});
  CHECK(scene.viewAt({10, 10}, &local) == &root);

  CoveringLayer dialog;
  scene.addLayer(dialog, Scene::kDialogOrder);
  CHECK(scene.viewAt({60, 60}, &local) == nullptr);
  scene.removeLayer(dialog);
  CHECK(scene.viewAt({60, 60}, &local) == &row);
}

TEST_CASE_FIXTURE(Tree, "the wheel scrolls the nearest view that takes it, down for a notch towards the user") {
  CHECK(pointer.handleEvent(wheelAt(60, 60, -1)));

  REQUIRE(list.scrolled.has_value());
  CHECK(list.scrolled->x == doctest::Approx(0));
  CHECK(list.scrolled->y == doctest::Approx(100));
}

TEST_CASE_FIXTURE(Tree, "a wheel turned over no scrolling view scrolls nothing") {
  CHECK(pointer.handleEvent(wheelAt(10, 10, -1)));

  CHECK(!list.scrolled.has_value());
}

TEST_CASE_FIXTURE(Tree, "a pointer over views with no event emitter moves, presses and leaves without an event") {
  SDL_Event motion{};
  motion.type = SDL_EVENT_MOUSE_MOTION;
  motion.motion.x = 60;
  motion.motion.y = 60;
  SDL_Event down{};
  down.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
  down.button.button = SDL_BUTTON_LEFT;
  down.button.x = 60;
  down.button.y = 60;
  SDL_Event up = down;
  up.type = SDL_EVENT_MOUSE_BUTTON_UP;

  CHECK(pointer.handleEvent(motion));
  CHECK(pointer.handleEvent(down));
  CHECK(pointer.handleEvent(up));
  pointer.pointerLeft();
}

TEST_CASE_FIXTURE(Tree, "an event that is not the pointer's is left to the host") {
  SDL_Event key{};
  key.type = SDL_EVENT_KEY_DOWN;

  CHECK(!pointer.handleEvent(key));
}

} // namespace rnwebos
