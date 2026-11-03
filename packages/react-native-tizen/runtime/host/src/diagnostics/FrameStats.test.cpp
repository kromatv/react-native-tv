#include "FrameStats.h"

#include <doctest/doctest.h>

#include <chrono>

namespace rntizen {

TEST_CASE("FrameStats takes the frame period from the median gap between ticks") {
  FrameStats stats(30);
  const auto start = FrameStats::Clock::now();
  const int gapsMs[] = {16, 17, 16, 40, 16, 17};
  auto now = start;

  stats.tick(now);
  for (const int gap : gapsMs) {
    now += std::chrono::milliseconds(gap);
    stats.tick(now);
  }
  const auto window = stats.take(now);

  CHECK(window.ticks == 7);
  CHECK(window.periodMs == doctest::Approx(16).epsilon(0.1));
}

TEST_CASE("FrameStats keeps its fallback period while too few ticks came") {
  FrameStats stats(20);
  const auto start = FrameStats::Clock::now();

  stats.tick(start);
  stats.tick(start + std::chrono::milliseconds(8));
  const auto window = stats.take(start + std::chrono::milliseconds(10));

  CHECK(window.periodMs == doctest::Approx(20));
}

} // namespace rntizen
