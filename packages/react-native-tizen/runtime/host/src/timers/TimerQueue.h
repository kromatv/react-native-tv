#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <vector>

namespace rntizen {

/// The timers a ThreadTimerRegistry waits on. Not thread-safe.
class TimerQueue final {
 public:
  using Clock = std::chrono::steady_clock;

  /// Schedules `timerID` `delay` after `now`, in place of any timer with that
  /// id. A recurring timer then repeats every `delay`, at least every 1 ms.
  void schedule(std::uint32_t timerID, std::chrono::microseconds delay, bool recurring, Clock::time_point now);
  void erase(std::uint32_t timerID);
  void clear();
  bool empty() const;

  /// The earliest deadline, `Clock::time_point::max()` when there is no timer.
  Clock::time_point nextDeadline() const;

  /// Takes the timers whose deadline is `now` or earlier, earliest deadline
  /// first, then by id; a recurring one is scheduled again from `now`.
  std::vector<std::uint32_t> expire(Clock::time_point now);

 private:
  struct Timer {
    Clock::time_point deadline;
    std::chrono::microseconds interval;
    bool recurring;
  };

  std::map<std::uint32_t, Timer> timers_;
};

} // namespace rntizen
