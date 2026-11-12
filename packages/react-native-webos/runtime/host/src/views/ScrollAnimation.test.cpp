#include "ScrollAnimation.h"

#include <doctest/doctest.h>

#include <chrono>
#include <thread>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::Point;

void runFrames(const ScrollAnimation &animation) {
  const auto giveUp = MainLoop::Clock::now() + std::chrono::seconds(3);
  while (animation.isRunning() && MainLoop::Clock::now() < giveUp) {
    MainLoop::get().iterate(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(4));
  }
}

} // namespace

TEST_CASE("a scroll animation decelerates along a quarter sine from one offset to the other") {
  const Point from{.x = 100, .y = 0};
  const Point to{.x = 300, .y = 50};

  CHECK(ScrollAnimation::offsetAt(from, to, 0) == from);
  CHECK(ScrollAnimation::offsetAt(from, to, 1) == to);
  CHECK(ScrollAnimation::offsetAt(from, to, 0.5).x == doctest::Approx(100 + 200 * 0.70711));
  CHECK(ScrollAnimation::offsetAt(from, to, 0.5).y == doctest::Approx(50 * 0.70711));
  CHECK(ScrollAnimation::offsetAt(from, to, 2) == to);
}

TEST_CASE("a scroll animation steps once a frame, never backwards, and ends on its target a quarter second on") {
  ScrollAnimation animation;
  std::vector<Point> steps;
  int finishes = 0;
  const auto started = MainLoop::Clock::now();

  animation.start({.x = 0, .y = 0}, {.x = 0, .y = 400}, [&](Point offset, bool finished) {
    steps.push_back(offset);
    if (finished) ++finishes;
  });
  CHECK(animation.isRunning());
  runFrames(animation);

  CHECK_FALSE(animation.isRunning());
  CHECK(MainLoop::Clock::now() - started >= std::chrono::milliseconds(250));
  REQUIRE(steps.size() >= 2);
  CHECK(finishes == 1);
  CHECK(steps.back().y == 400);
  for (std::size_t index = 1; index < steps.size(); ++index) CHECK(steps[index].y >= steps[index - 1].y);
  CHECK(steps.front().y < 400);
}

TEST_CASE("a stopped scroll animation takes no more steps, and starting again replaces the one running") {
  ScrollAnimation animation;
  int first = 0;
  int second = 0;

  animation.start({}, {.x = 100, .y = 0}, [&](Point, bool) { ++first; });
  animation.start({}, {.x = 50, .y = 0}, [&](Point, bool) { ++second; });
  MainLoop::get().iterate(true);
  CHECK(first == 0);
  CHECK(second == 1);

  animation.stop();
  MainLoop::get().iterate(true);
  CHECK_FALSE(animation.isRunning());
  CHECK(second == 1);
}

} // namespace rnwebos
