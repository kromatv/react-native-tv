#pragma once

#include <Ecore.h>

#include <atomic>
#include <chrono>
#include <thread>

namespace rntizen::testing {

class MainLoopScope final {
 public:
  MainLoopScope() {
    static const int initialized = ecore_init();
    static_cast<void>(initialized);
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
    ecore_main_loop_iterate();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
};

} // namespace rntizen::testing
