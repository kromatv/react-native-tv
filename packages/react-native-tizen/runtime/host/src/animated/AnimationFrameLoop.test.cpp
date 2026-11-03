#include "AnimationFrameLoop.h"

#include <doctest/doctest.h>

#include <Ecore.h>

#include <memory>

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

} // namespace rntizen
