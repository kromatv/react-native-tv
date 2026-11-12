#include "AnimationFrameLoop.h"

#include "../threading/MainLoop.test.h"
#include "../threading/MainThread.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace rnwebos {

namespace {

using std::chrono::milliseconds;
using testing::MainLoopScope;

} // namespace

TEST_CASE("AnimationFrameLoop calls back on every frame until it is stopped") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->start([&frames] { ++frames; });
  mainLoop.runFor(milliseconds(200));
  loop->stop();
  const int counted = frames;
  mainLoop.runFor(milliseconds(100));

  CHECK(counted > 0);
  CHECK(frames == counted);
}

TEST_CASE("AnimationFrameLoop stops by itself once what it follows is gone") {
  const MainLoopScope mainLoop;
  auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);
  owner.reset();

  loop->start([&frames] { ++frames; });
  mainLoop.runFor(milliseconds(150));

  CHECK(frames == 0);
}

TEST_CASE("AnimationFrameLoop removes its frame callback when destroyed while running") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  int frames = 0;
  {
    const auto loop = std::make_shared<AnimationFrameLoop>();
    loop->follow(owner);
    loop->start([&frames] { ++frames; });
  }

  mainLoop.runFor(milliseconds(100));

  CHECK(frames == 0);
}

TEST_CASE("AnimationFrameLoop started and stopped from another thread runs its frames on the main loop") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  std::atomic<int> frames{0};
  std::atomic<bool> onMainLoop{true};
  loop->follow(owner);

  std::thread([&loop, &frames, &onMainLoop] {
    loop->start([&frames, &onMainLoop] {
      ++frames;
      onMainLoop = onMainLoop && isMainThread();
    });
  }).join();
  mainLoop.runFor(milliseconds(200));
  std::thread([&loop] { loop->stop(); }).join();
  mainLoop.runFor(milliseconds(50));
  const int counted = frames;
  mainLoop.runFor(milliseconds(100));

  CHECK(counted > 0);
  CHECK(frames == counted);
  CHECK(onMainLoop);
}

TEST_CASE("AnimationFrameLoop stopped from its own frame runs no other") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->start([&frames, &loop] {
    ++frames;
    loop->stop();
  });
  mainLoop.runFor(milliseconds(200));

  CHECK(frames == 1);
}

TEST_CASE("AnimationFrameLoop started again while it runs calls the newest callback") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int first = 0;
  int second = 0;
  loop->follow(owner);

  loop->start([&first] { ++first; });
  loop->start([&second] { ++second; });
  mainLoop.runFor(milliseconds(200));
  loop->stop();

  CHECK(first == 0);
  CHECK(second > 0);
}

TEST_CASE("AnimationFrameLoop stopped before it starts runs its frames once started") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->stop();
  loop->start([&frames] { ++frames; });
  mainLoop.runFor(milliseconds(100));
  loop->stop();

  CHECK(frames > 0);
}

TEST_CASE("AnimationFrameLoop started with an empty callback skips its frames rather than calling it") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->start({});
  mainLoop.runFor(milliseconds(100));
  loop->start([&frames] { ++frames; });
  mainLoop.runFor(milliseconds(100));
  loop->stop();

  CHECK(frames > 0);
}

TEST_CASE("AnimationFrameLoop destroyed on another thread removes its frame callback on the main loop") {
  const MainLoopScope mainLoop;
  const auto owner = std::make_shared<int>(0);
  auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);
  loop->start([&frames] { ++frames; });

  std::thread([destroyed = std::move(loop)]() mutable { destroyed.reset(); }).join();
  mainLoop.runFor(milliseconds(100));

  CHECK(frames == 0);
}

} // namespace rnwebos
