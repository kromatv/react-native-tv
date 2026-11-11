#include "../threading/MainLoop.test.h"
#include "ThreadTimerRegistry.h"

#include <Ecore.h>
#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;

struct Timers {
  Timers() {
    auto owned = std::make_unique<ThreadTimerRegistry>();
    registry = owned.get();
    manager = std::make_shared<facebook::react::TimerManager>(std::move(owned));
    registry->setTimerManager(manager);
    manager->setRuntimeExecutor([this](std::function<void(facebook::jsi::Runtime &)> &&) { ++calls; });
  }

  ~Timers() { manager.reset(); }

  Timers(const Timers &) = delete;
  Timers &operator=(const Timers &) = delete;

  bool waitForCalls(int count, bool iterateMainLoop) const {
    const auto giveUp = std::chrono::steady_clock::now() + 5s;
    while (calls < count && std::chrono::steady_clock::now() < giveUp) {
      if (iterateMainLoop) ecore_main_loop_iterate();
      std::this_thread::sleep_for(1ms);
    }
    return calls == count;
  }

  testing::MainLoopScope loop;
  std::atomic<int> calls{0};
  ThreadTimerRegistry *registry{nullptr};
  std::shared_ptr<facebook::react::TimerManager> manager;
};

} // namespace

TEST_CASE("expired timers fire from the timer thread when no frame comes") {
  Timers timers;

  timers.registry->createTimer(1, 5);
  timers.registry->createTimer(2, 0);

  CHECK(timers.waitForCalls(2, false));
}

TEST_CASE("expired timers fire on the next frame of the main loop") {
  Timers timers;

  timers.registry->createTimer(1, 0);

  CHECK(timers.waitForCalls(1, true));
}

TEST_CASE("a recurring timer fires until it is deleted") {
  Timers timers;

  timers.registry->createRecurringTimer(3, 1);
  CHECK(timers.waitForCalls(2, true));
  timers.registry->deleteTimer(3);
  const int fired = timers.calls;
  std::this_thread::sleep_for(150ms);

  CHECK(timers.calls == fired);
}

TEST_CASE("a deleted timer never fires") {
  Timers timers;

  timers.registry->createTimer(1, 20);
  timers.registry->deleteTimer(1);
  std::this_thread::sleep_for(150ms);

  CHECK(timers.calls == 0);
}

TEST_CASE("a timer created after quit never fires") {
  Timers timers;
  timers.registry->quit();

  timers.registry->createTimer(1, 0);
  std::this_thread::sleep_for(150ms);

  CHECK(timers.calls == 0);
}

} // namespace rntizen
