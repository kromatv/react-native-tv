#pragma once

#include "platform/MainLoop.h"

#include <chrono>
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
    id_ = MainLoop::get().addTimer(delay, [this, task = std::move(task)] {
      id_ = 0;
      task();
    });
  }

  void cancel() {
    if (id_ != 0) MainLoop::get().removeTimer(std::exchange(id_, 0));
  }

  bool isPending() const {
    return id_ != 0;
  }

 private:
  MainLoop::Id id_{0};
};

} // namespace rnwebos::expo
