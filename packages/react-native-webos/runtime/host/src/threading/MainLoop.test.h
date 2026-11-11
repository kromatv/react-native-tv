#pragma once

#include "../platform/MainLoop.h"

#include <atomic>
#include <chrono>
#include <thread>

namespace rnwebos::testing {

/// Runs the main loop from a test: the thread that makes the first one is the
/// main thread for the whole test binary.
class MainLoopScope final {
 public:
  MainLoopScope() {
    MainLoop::get();
  }

  ~MainLoopScope() {
    const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
    while (std::chrono::steady_clock::now() < until) iterate();
  }

  MainLoopScope(const MainLoopScope &) = delete;
  MainLoopScope &operator=(const MainLoopScope &) = delete;

  bool runUntil(const std::atomic<bool> &done) const {
    const auto giveUp = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!done && std::chrono::steady_clock::now() < giveUp) iterate();
    return done;
  }

 private:
  static void iterate() {
    MainLoop::get().iterate(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
};

} // namespace rnwebos::testing
