#include "MainLoop.h"

#include "../threading/MainLoop.test.h"

#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

namespace rnwebos {

namespace {

using std::chrono::milliseconds;
using testing::MainLoopScope;

} // namespace

TEST_CASE("MainLoop runs a posted task in a later iteration, in the order posted, never inline") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::vector<int> order;

  loop.post([&order, &loop] {
    order.push_back(1);
    loop.post([&order] { order.push_back(3); });
  });
  loop.post([&order] { order.push_back(2); });
  CHECK(order.empty());
  loop.iterate();

  CHECK(order == std::vector<int>{1, 2});
  loop.iterate();
  CHECK(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("MainLoop runs what another thread posts on the main thread") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::atomic<bool> ran{false};
  std::atomic<bool> onMainThread{false};
  std::atomic<bool> workerIsMain{true};

  std::thread([&] {
    workerIsMain = loop.isMainThread();
    loop.post([&] {
      onMainThread = loop.isMainThread();
      ran = true;
    });
  }).join();

  CHECK(scope.runUntil(ran));
  CHECK(onMainThread);
  CHECK_FALSE(workerIsMain);
  CHECK(loop.isMainThread());
}

TEST_CASE("MainLoop runs a timer once, after its delay, and never a removed one") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  int fired = 0;
  int removed = 0;
  const auto started = MainLoop::Clock::now();
  MainLoop::Clock::duration waited{};

  loop.addTimer(milliseconds(30), [&] {
    ++fired;
    waited = MainLoop::Clock::now() - started;
  });
  const auto cancelled = loop.addTimer(milliseconds(10), [&removed] { ++removed; });
  loop.removeTimer(cancelled);
  loop.iterate();
  CHECK(fired == 0);
  scope.runFor(milliseconds(120));

  CHECK(fired == 1);
  CHECK(waited >= milliseconds(30));
  CHECK(removed == 0);
}

TEST_CASE("MainLoop drops a due timer another timer removed in the same iteration") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  int second = 0;
  MainLoop::Id secondTimer = 0;

  loop.addTimer(milliseconds(1), [&loop, &secondTimer] { loop.removeTimer(secondTimer); });
  secondTimer = loop.addTimer(milliseconds(1), [&second] { ++second; });
  scope.runFor(milliseconds(40));

  CHECK(second == 0);
}

TEST_CASE("MainLoop lets a timer add another, which waits for its own delay") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::atomic<bool> chained{false};

  loop.addTimer(milliseconds(1), [&loop, &chained] { loop.addTimer(milliseconds(1), [&chained] { chained = true; }); });

  CHECK(scope.runUntil(chained));
}

TEST_CASE("MainLoop calls a frame callback once per frame, with the frame's time, until it is removed") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::vector<MainLoop::Clock::time_point> frames;

  const auto id = loop.addFrameCallback([&frames](MainLoop::Clock::time_point time) { frames.push_back(time); });
  loop.iterate();
  CHECK(frames.empty());
  loop.iterate(true);
  loop.iterate(true);
  loop.removeFrameCallback(id);
  loop.iterate(true);

  REQUIRE(frames.size() == 2);
  CHECK(frames[1] >= frames[0]);
}

TEST_CASE("MainLoop skips a frame callback removed during the frame, and starts one added during it at the next") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  int removedCalls = 0;
  int addedCalls = 0;
  int selfRemovingCalls = 0;
  MainLoop::Id removed = 0;
  MainLoop::Id added = 0;
  MainLoop::Id selfRemoving = 0;

  const auto first = loop.addFrameCallback([&](MainLoop::Clock::time_point) {
    loop.removeFrameCallback(removed);
    if (added == 0) added = loop.addFrameCallback([&addedCalls](MainLoop::Clock::time_point) { ++addedCalls; });
  });
  removed = loop.addFrameCallback([&removedCalls](MainLoop::Clock::time_point) { ++removedCalls; });
  selfRemoving = loop.addFrameCallback([&](MainLoop::Clock::time_point) {
    ++selfRemovingCalls;
    loop.removeFrameCallback(selfRemoving);
  });
  loop.iterate(true);

  CHECK(removedCalls == 0);
  CHECK(addedCalls == 0);
  CHECK(selfRemovingCalls == 1);
  loop.iterate(true);
  CHECK(addedCalls == 1);
  CHECK(selfRemovingCalls == 1);
  loop.removeFrameCallback(first);
  loop.removeFrameCallback(added);
}

TEST_CASE("MainLoop draws a requested frame after the frame's callbacks, and returns the code it quit with") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::vector<const char *> order;
  const auto callback = loop.addFrameCallback([&order](MainLoop::Clock::time_point) { order.push_back("callback"); });
  loop.setRenderer([&order, &loop] {
    order.push_back("render");
    loop.quit(7);
    return true;
  });

  loop.requestFrame();
  const int exitCode = loop.run();
  loop.setRenderer(nullptr);
  loop.removeFrameCallback(callback);

  CHECK(exitCode == 7);
  CHECK(order == std::vector<const char *>{"callback", "render"});
}

TEST_CASE("MainLoop keeps running frames a period apart, however late its timer wakes it") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  const auto period = loop.framePeriod();
  constexpr std::chrono::microseconds kPeriod{10'400};
  constexpr std::size_t kFrames = 60;
  loop.setFramePeriod(kPeriod);
  std::vector<MainLoop::Clock::time_point> times;
  const auto callback = loop.addFrameCallback([&times, &loop](MainLoop::Clock::time_point frameTime) {
    times.push_back(frameTime);
    if (times.size() == kFrames) loop.quit();
  });

  loop.run();
  loop.removeFrameCallback(callback);
  loop.setFramePeriod(period);

  REQUIRE(times.size() == kFrames);
  std::vector<double> gaps;
  for (std::size_t index = 1; index < times.size(); ++index) {
    gaps.push_back(std::chrono::duration<double, std::milli>(times[index] - times[index - 1]).count());
  }
  // Another process on a busy machine holds a frame up now and then: the
  // longest tenth of the gaps is left out.
  std::ranges::sort(gaps);
  gaps.resize(gaps.size() - gaps.size() / 10);
  double total = 0;
  for (const double gap : gaps) total += gap;
  const double typicalGap = total / static_cast<double>(gaps.size());
  CAPTURE(typicalGap);
  CHECK(typicalGap < 10.7);
  CHECK(typicalGap > 9.0);
}

TEST_CASE("MainLoop wakes from its wait for a task posted by another thread, and tells its wait observer") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::vector<bool> waits;
  loop.setWaitObserver([&waits](bool waiting) { waits.push_back(waiting); });

  std::thread poster([&loop] {
    std::this_thread::sleep_for(milliseconds(30));
    loop.post([&loop] { loop.quit(); });
  });
  const int exitCode = loop.run();
  poster.join();
  loop.setWaitObserver(nullptr);

  CHECK(exitCode == 0);
  REQUIRE(waits.size() >= 2);
  CHECK(waits[0]);
  CHECK_FALSE(waits[1]);
}

TEST_CASE("MainLoop hands SDL's events to its event handler") {
  const MainLoopScope scope;
  auto &loop = MainLoop::get();
  std::vector<SDL_Scancode> keys;
  loop.setEventHandler([&keys](const SDL_Event &event) {
    if (event.type == SDL_EVENT_KEY_DOWN) keys.push_back(event.key.scancode);
  });
  SDL_Event event{};
  event.type = SDL_EVENT_KEY_DOWN;
  event.key.scancode = SDL_SCANCODE_DOWN;

  SDL_PushEvent(&event);
  loop.iterate();
  loop.setEventHandler(nullptr);
  SDL_PushEvent(&event);
  loop.iterate();

  CHECK(keys == std::vector<SDL_Scancode>{SDL_SCANCODE_DOWN});
}

TEST_CASE("MainLoop keeps its frame period when asked for one that is not positive") {
  auto &loop = MainLoop::get();
  const auto period = loop.framePeriod();

  loop.setFramePeriod(milliseconds(8));
  CHECK(loop.framePeriod() == milliseconds(8));
  loop.setFramePeriod(MainLoop::Clock::duration::zero());
  CHECK(loop.framePeriod() == milliseconds(8));

  loop.setFramePeriod(period);
}

} // namespace rnwebos
