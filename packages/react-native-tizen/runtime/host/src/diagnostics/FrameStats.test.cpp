#include "FrameStats.h"

#include <doctest/doctest.h>

#include <chrono>

namespace rntizen {

namespace {

using namespace std::chrono_literals;

constexpr double kPeriodMs = 16;

} // namespace

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

TEST_CASE("FrameStats keeps the frame period between 4 and 50 ms") {
  const auto start = FrameStats::Clock::now();
  FrameStats fast(1);
  FrameStats slow(100);
  FrameStats measured(kPeriodMs);

  for (int tick = 0; tick < 6; ++tick) measured.tick(start + tick * 200ms);

  CHECK(fast.take(start).periodMs == doctest::Approx(4));
  CHECK(slow.take(start).periodMs == doctest::Approx(50));
  CHECK(measured.take(start + 1s).periodMs == doctest::Approx(50));
}

TEST_CASE("FrameStats counts the display ticks the loop missed as dropped once the canvas renders") {
  FrameStats stats(kPeriodMs);
  const auto start = FrameStats::Clock::now();

  stats.tick(start);
  stats.tick(start + 16ms);
  stats.tick(start + 64ms);
  stats.renderStarted(start + 65ms);
  stats.renderFinished(start + 69ms);
  stats.tick(start + 80ms);
  stats.tick(start + 128ms);
  stats.tick(start + 144ms);
  const auto window = stats.take(start + 150ms);

  CHECK(window.ticks == 6);
  CHECK(window.frames == 1);
  CHECK(window.dropped == 2);
}

TEST_CASE("FrameStats averages a window's renders and keeps the longest") {
  FrameStats stats(kPeriodMs);
  const auto start = FrameStats::Clock::now();

  stats.renderFinished(start);
  stats.renderStarted(start);
  stats.renderFinished(start + 4ms);
  stats.renderStarted(start + 10ms);
  stats.renderFinished(start + 18ms);
  const auto window = stats.take(start + 20ms);
  const auto next = stats.take(start + 40ms);

  CHECK(window.frames == 2);
  CHECK(window.renderAvgMs == doctest::Approx(6));
  CHECK(window.renderMaxMs == doctest::Approx(8));
  CHECK(next.frames == 0);
  CHECK(next.renderAvgMs == doctest::Approx(0));
}

TEST_CASE("FrameStats adds up the time the main loop is awake, splitting a wake across windows") {
  FrameStats stats(kPeriodMs);
  const auto start = FrameStats::Clock::now();

  stats.sleeping(start);
  stats.woke(start);
  stats.sleeping(start + 5ms);
  stats.sleeping(start + 6ms);
  stats.woke(start + 10ms);
  const auto first = stats.take(start + 13ms);
  stats.sleeping(start + 15ms);
  const auto second = stats.take(start + 20ms);

  CHECK(first.mainBusyMs == doctest::Approx(8));
  CHECK(second.mainBusyMs == doctest::Approx(2));
}

} // namespace rntizen
