#include "../threading/MainLoop.test.h"
#include "FrameClock.h"

#include <doctest/doctest.h>

#include <atomic>
#include <memory>
#include <stdexcept>

namespace rntizen {

TEST_CASE("a frame that throws is logged and the clock keeps ticking") {
  std::atomic<int> frames{0};
  std::atomic<bool> thirdFrame{false};
  const testing::MainLoopScope loop;
  const auto clock = std::make_shared<FrameClock>([&frames, &thirdFrame] {
    const int frame = ++frames;
    if (frame == 1) throw std::runtime_error("boom");
    if (frame == 2) throw 7;
    thirdFrame = true;
    return false;
  });

  clock->request();
  loop.runUntil(thirdFrame);
  clock->close();

  CHECK(thirdFrame);
}

TEST_CASE("a closed clock never calls its frame again") {
  std::atomic<int> frames{0};
  const testing::MainLoopScope loop;
  const auto clock = std::make_shared<FrameClock>([&frames] {
    ++frames;
    return true;
  });

  clock->close();
  clock->request();

  CHECK(frames == 0);
}

} // namespace rntizen
