#include "FrameTicker.h"

#include <doctest/doctest.h>

#include <Ecore.h>

#include <memory>

namespace rntizen {

namespace {

/// An Ecore main loop whose animators tick only when the test says so.
struct ManualTicks {
  ManualTicks() {
    ecore_init();
    ecore_animator_source_set(ECORE_ANIMATOR_SOURCE_CUSTOM);
  }
  ~ManualTicks() {
    ecore_animator_source_set(ECORE_ANIMATOR_SOURCE_TIMER);
    ecore_shutdown();
  }
  ManualTicks(const ManualTicks &) = delete;
  ManualTicks &operator=(const ManualTicks &) = delete;

  /// One turn of the loop in which the display ticked `ticks` times.
  void turn(int ticks) const {
    for (int tick = 0; tick < ticks; ++tick) ecore_animator_custom_tick();
    ecore_main_loop_iterate();
  }
};

} // namespace

TEST_CASE("FrameTicker calls back once in a turn of the loop, however many times the display ticked") {
  const ManualTicks loop;
  FrameTicker ticker;
  int frames = 0;
  ticker.start([&frames] { ++frames; });

  loop.turn(5);
  CHECK(frames == 1);

  loop.turn(1);
  CHECK(frames == 2);
}

TEST_CASE("FrameTicker waits for a tick: a turn of the loop without one draws no frame") {
  const ManualTicks loop;
  FrameTicker ticker;
  int frames = 0;
  ticker.start([&frames] { ++frames; });

  loop.turn(0);
  loop.turn(0);

  CHECK(frames == 0);
}

TEST_CASE("FrameTicker stopped calls back no more, and runs again once started") {
  const ManualTicks loop;
  FrameTicker ticker;
  int frames = 0;
  ticker.start([&frames] { ++frames; });
  loop.turn(1);

  ticker.stop();
  loop.turn(3);
  CHECK(frames == 1);
  CHECK_FALSE(ticker.isRunning());

  ticker.start([&frames] { ++frames; });
  loop.turn(1);
  CHECK(frames == 2);
}

TEST_CASE("FrameTicker started again while it runs calls the newest callback") {
  const ManualTicks loop;
  FrameTicker ticker;
  int first = 0;
  int second = 0;

  ticker.start([&first] { ++first; });
  ticker.start([&second] { ++second; });
  loop.turn(2);

  CHECK(first == 0);
  CHECK(second == 1);
}

TEST_CASE("FrameTicker stopped or destroyed from its own callback runs no other frame") {
  const ManualTicks loop;
  FrameTicker stopped;
  auto destroyed = std::make_unique<FrameTicker>();
  int frames = 0;
  stopped.start([&frames, &stopped] {
    ++frames;
    stopped.stop();
  });
  destroyed->start([&frames, &destroyed] {
    ++frames;
    destroyed.reset();
  });

  loop.turn(1);
  loop.turn(1);

  CHECK(frames == 2);
}

} // namespace rntizen
