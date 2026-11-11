#include "ScrollView.h"

#include "../render/RasterScene.test.h"
#include "ViewFixtures.test.h"

#include <doctest/doctest.h>

#include <react/renderer/components/scrollview/ScrollViewProps.h>

#include <chrono>
#include <memory>
#include <thread>

namespace rnwebos {

namespace {

using facebook::react::Point;
using facebook::react::ScrollViewProps;
using facebook::react::ScrollViewState;
using testing::filled;
using testing::frameOf;
using testing::place;
using testing::propsWith;

facebook::react::State::Shared contentOf(facebook::react::Float width, facebook::react::Float height) {
  return std::make_shared<const facebook::react::ScrollViewShadowNode::ConcreteState>(
      std::make_shared<const ScrollViewState>(Point{}, facebook::react::Rect{.origin = {}, .size = {width, height}}, 0),
      facebook::react::ShadowNodeFamily::Weak{});
}

facebook::react::Props::Shared scrolling() {
  return propsWith<ScrollViewProps>([](ScrollViewProps &props) { props.yogaStyle.setOverflow(facebook::yoga::Overflow::Scroll); });
}

// A 200x100 scroll view over content 1000 tall: red at the top, green in the
// second hundred, blue in the last, and a white band 800 wide for the
// horizontal case.
struct List {
  explicit List(facebook::react::Float contentWidth = 200, facebook::react::Float contentHeight = 1000) {
    place(scroll, scrolling(), frameOf(0, 0, 200, 100));
    scroll.updateState(contentOf(contentWidth, contentHeight));
    place(content, std::make_shared<facebook::react::ViewProps>(), frameOf(0, 0, contentWidth, contentHeight));
    place(top, filled(255, 0, 0), frameOf(0, 0, 200, 100));
    place(second, filled(0, 255, 0), frameOf(0, 100, 200, 100));
    place(last, filled(0, 0, 255), frameOf(0, 900, 200, 100));
    place(far, filled(255, 255, 255), frameOf(600, 0, 200, 100));
    content.mountChild(top, 0);
    content.mountChild(second, 1);
    content.mountChild(last, 2);
    content.mountChild(far, 3);
    scroll.mountChild(content, 0);
    root->mountChild(scroll, 0);
  }

  SkColor shown() {
    frame.draw();
    return frame.pixel(100, 50);
  }

  void command(const char *name, folly::dynamic args) {
    scroll.handleCommand(name, args);
  }

  void runUntilStill(std::chrono::milliseconds longest = std::chrono::milliseconds(2000)) {
    const auto giveUp = MainLoop::Clock::now() + longest;
    auto last = scroll.contentOffset();
    auto still = MainLoop::Clock::now();
    while (MainLoop::Clock::now() < giveUp && MainLoop::Clock::now() - still < std::chrono::milliseconds(150)) {
      MainLoop::get().iterate(true);
      std::this_thread::sleep_for(std::chrono::milliseconds(4));
      if (scroll.contentOffset() != last) {
        last = scroll.contentOffset();
        still = MainLoop::Clock::now();
      }
    }
  }

  testing::RasterScene frame{PixelSize{200, 100}};
  std::unique_ptr<ComponentView> root{testing::rootOf(frame.scene())};
  ScrollView scroll{frame.scene(), 2};
  ComponentView content{frame.scene(), 3};
  ComponentView top{frame.scene(), 4};
  ComponentView second{frame.scene(), 5};
  ComponentView last{frame.scene(), 6};
  ComponentView far{frame.scene(), 7};
};

} // namespace

TEST_CASE("scrollTo without animation shifts the content at once") {
  List list;
  CHECK(list.shown() == SK_ColorRED);

  list.command("scrollTo", folly::dynamic::array(0, 100, false));

  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 100});
  CHECK(list.shown() == SK_ColorGREEN);
}

TEST_CASE("a scroll stops at the content's edges, and scrollToEnd goes to the last of it") {
  List list;

  list.command("scrollTo", folly::dynamic::array(0, 5000, false));
  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 900});
  CHECK(list.shown() == SK_ColorBLUE);

  list.command("scrollTo", folly::dynamic::array(-50, -50, false));
  CHECK(list.scroll.contentOffset() == Point{});

  list.command("scrollToEnd", folly::dynamic::array(false));
  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 900});
}

TEST_CASE("scrollToEnd follows the axis the content overflows on") {
  List list(800, 100);

  list.command("scrollToEnd", folly::dynamic::array(false));

  CHECK(list.scroll.contentOffset() == Point{.x = 600, .y = 0});
  CHECK(list.shown() == SK_ColorWHITE);
}

TEST_CASE("an animated scrollTo passes between the two offsets and arrives") {
  List list;

  list.command("scrollTo", folly::dynamic::array(0, 900, true));
  CHECK(list.scroll.contentOffset() == Point{});
  MainLoop::get().iterate(true);
  std::this_thread::sleep_for(std::chrono::milliseconds(40));
  MainLoop::get().iterate(true);
  const auto midway = list.scroll.contentOffset().y;
  list.runUntilStill();

  CHECK(midway > 0);
  CHECK(midway < 900);
  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 900});
  CHECK(list.shown() == SK_ColorBLUE);
}

TEST_CASE("scrollBy moves the content by a step and says when it could not") {
  List list;

  CHECK(list.scroll.scrollBy(0, 100));
  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 100});
  CHECK(list.shown() == SK_ColorGREEN);
  CHECK(list.scroll.scrollBy(0, -500));
  CHECK(list.scroll.contentOffset() == Point{});
  CHECK_FALSE(list.scroll.scrollBy(0, -1));
  CHECK_FALSE(list.scroll.scrollBy(40, 0));
  list.runUntilStill(std::chrono::milliseconds(400));
  CHECK(list.scroll.contentOffset() == Point{});
}

TEST_CASE("scrollBy takes over from an animated scroll where it is") {
  List list;

  list.command("scrollTo", folly::dynamic::array(0, 900, true));
  MainLoop::get().iterate(true);
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  MainLoop::get().iterate(true);
  const auto reached = list.scroll.contentOffset().y;
  list.scroll.scrollBy(0, 10);
  list.runUntilStill(std::chrono::milliseconds(600));

  CHECK(list.scroll.contentOffset().y == doctest::Approx(reached + 10));
}

TEST_CASE("the offset follows the contentOffset prop and comes back inside content that shrank") {
  List list;

  list.scroll.updateProps(propsWith<ScrollViewProps>([](ScrollViewProps &props) {
    props.yogaStyle.setOverflow(facebook::yoga::Overflow::Scroll);
    props.contentOffset = {.x = 0, .y = 900};
  }));
  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 900});

  list.scroll.updateState(contentOf(200, 300));
  CHECK(list.scroll.contentOffset() == Point{.x = 0, .y = 200});
}

TEST_CASE("a scroll view ignores commands it does not have and malformed arguments") {
  List list;

  list.command("flashScrollIndicators", folly::dynamic::array());
  list.command("scrollTo", folly::dynamic::array(0));
  list.command("scrollTo", folly::dynamic::object("y", 100));
  list.command("scrollTo", folly::dynamic::array("a", "b", "c"));

  CHECK(list.scroll.contentOffset() == Point{});
}

} // namespace rnwebos
