#include "AnimationFrameLoop.h"

#include "../threading/MainThread.h"

#include <doctest/doctest.h>

#include <Ecore.h>

#include <atomic>
#include <memory>
#include <thread>

namespace rntizen {

namespace {

void runMainLoopFor(double seconds) {
  ecore_timer_add(
      seconds,
      [](void *) -> Eina_Bool {
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
      },
      nullptr);
  ecore_main_loop_begin();
}

struct EcoreLoop {
  EcoreLoop() {
    ecore_init();
  }
  ~EcoreLoop() {
    ecore_shutdown();
  }
  EcoreLoop(const EcoreLoop &) = delete;
  EcoreLoop &operator=(const EcoreLoop &) = delete;
};

} // namespace

TEST_CASE("AnimationFrameLoop calls back on every frame until it is stopped") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->start([&frames] { ++frames; });
  runMainLoopFor(0.2);
  loop->stop();
  const int counted = frames;
  runMainLoopFor(0.1);

  CHECK(counted > 0);
  CHECK(frames == counted);
}

TEST_CASE("AnimationFrameLoop stops by itself once what it follows is gone") {
  const EcoreLoop ecore;
  auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);
  owner.reset();

  loop->start([&frames] { ++frames; });
  runMainLoopFor(0.15);

  CHECK(frames == 0);
}

TEST_CASE("AnimationFrameLoop removes its animator when destroyed while running") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  int frames = 0;
  {
    const auto loop = std::make_shared<AnimationFrameLoop>();
    loop->follow(owner);
    loop->start([&frames] { ++frames; });
  }

  runMainLoopFor(0.1);

  CHECK(frames == 0);
}

TEST_CASE("AnimationFrameLoop started and stopped from another thread runs its frames on the main loop") {
  const EcoreLoop ecore;
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
  runMainLoopFor(0.2);
  std::thread([&loop] { loop->stop(); }).join();
  runMainLoopFor(0.05);
  const int counted = frames;
  runMainLoopFor(0.1);

  CHECK(counted > 0);
  CHECK(frames == counted);
  CHECK(onMainLoop);
}

TEST_CASE("AnimationFrameLoop stopped from its own frame runs no other") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->start([&frames, &loop] {
    ++frames;
    loop->stop();
  });
  runMainLoopFor(0.2);

  CHECK(frames == 1);
}

TEST_CASE("AnimationFrameLoop started again while it runs calls the newest callback") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int first = 0;
  int second = 0;
  loop->follow(owner);

  loop->start([&first] { ++first; });
  loop->start([&second] { ++second; });
  runMainLoopFor(0.2);
  loop->stop();

  CHECK(first == 0);
  CHECK(second > 0);
}

TEST_CASE("AnimationFrameLoop stopped before it starts runs its frames once started") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->stop();
  loop->start([&frames] { ++frames; });
  runMainLoopFor(0.1);
  loop->stop();

  CHECK(frames > 0);
}

TEST_CASE("AnimationFrameLoop started with an empty callback skips its frames rather than calling it") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  const auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);

  loop->start({});
  runMainLoopFor(0.1);
  loop->start([&frames] { ++frames; });
  runMainLoopFor(0.1);
  loop->stop();

  CHECK(frames > 0);
}

TEST_CASE("AnimationFrameLoop destroyed on another thread removes its animator on the main loop") {
  const EcoreLoop ecore;
  const auto owner = std::make_shared<int>(0);
  auto loop = std::make_shared<AnimationFrameLoop>();
  int frames = 0;
  loop->follow(owner);
  loop->start([&frames] { ++frames; });

  std::thread([destroyed = std::move(loop)]() mutable { destroyed.reset(); }).join();
  runMainLoopFor(0.1);

  CHECK(frames == 0);
}

} // namespace rntizen
