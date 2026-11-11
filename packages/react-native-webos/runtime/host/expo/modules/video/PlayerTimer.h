#pragma once

#include "platform/MainLoop.h"

#include <chrono>
#include <memory>
#include <utility>

namespace rnwebos::expo {

/// One task the main loop runs after a delay, unless it is cancelled,
/// started again or destroyed first. Main thread only.
class PlayerTimer final {
 public:
  PlayerTimer() = default;
  ~PlayerTimer() {
    cancel();
  }

  PlayerTimer(const PlayerTimer &) = delete;
  PlayerTimer &operator=(const PlayerTimer &) = delete;

  void start(std::chrono::milliseconds delay, MainLoop::Task task) {
    cancel();
    armed_ = std::make_shared<bool>(true);
    MainLoop::get().addTimer(delay, [armed = armed_, task = std::move(task)] {
      if (std::exchange(*armed, false)) task();
    });
  }

  // Disarmed rather than removed from the loop: a player can be destroyed by
  // the closing main loop itself, which nothing may then call back into.
  void cancel() {
    if (armed_) *armed_ = false;
    armed_.reset();
  }

 private:
  std::shared_ptr<bool> armed_;
};

} // namespace rnwebos::expo
