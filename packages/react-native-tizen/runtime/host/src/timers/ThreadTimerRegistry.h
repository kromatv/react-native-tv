#pragma once

#include "FrameClock.h"

#include <react/runtime/PlatformTimerRegistry.h>
#include <react/runtime/TimerManager.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

namespace rntizen {

/// Backs `setTimeout`, `setInterval` and `requestAnimationFrame`, which React
/// Native's TimerManager makes a 0 ms timer. One thread waits for deadlines; a
/// timer that expires fires on the next display frame, from a FrameClock, as
/// iOS fires timers from a CADisplayLink and Android from the Choreographer. A
/// timer no frame has fired 100 ms after it expired, as in a window the
/// compositor sends no frames to, fires from the thread. Fired timers go to
/// the TimerManager, which runs their callbacks on the JavaScript thread.
class ThreadTimerRegistry final : public facebook::react::PlatformTimerRegistry {
 public:
  ThreadTimerRegistry();
  ~ThreadTimerRegistry() noexcept override;

  ThreadTimerRegistry(const ThreadTimerRegistry &) = delete;
  ThreadTimerRegistry &operator=(const ThreadTimerRegistry &) = delete;

  void createTimer(uint32_t timerID, double delayMS) override;
  void createRecurringTimer(uint32_t timerID, double delayMS) override;
  void deleteTimer(uint32_t timerID) override;
  void quit() override;
  void setTimerManager(std::weak_ptr<facebook::react::TimerManager> timerManager) override;

 private:
  struct State;

  void schedule(uint32_t timerID, double delayMS, bool recurring);
  void run();

  const std::shared_ptr<State> state_;
  const std::shared_ptr<FrameClock> frames_;
  std::thread thread_;
};

} // namespace rntizen
