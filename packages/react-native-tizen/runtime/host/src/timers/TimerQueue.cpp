#include "TimerQueue.h"

#include <algorithm>
#include <utility>

namespace rntizen {

namespace {

constexpr std::chrono::microseconds kMinimumInterval = std::chrono::milliseconds(1);

} // namespace

void TimerQueue::schedule(std::uint32_t timerID, std::chrono::microseconds delay, bool recurring, Clock::time_point now) {
  timers_.insert_or_assign(timerID, Timer{.deadline = now + delay, .interval = delay, .recurring = recurring});
}

void TimerQueue::erase(std::uint32_t timerID) {
  timers_.erase(timerID);
}

void TimerQueue::clear() {
  timers_.clear();
}

bool TimerQueue::empty() const {
  return timers_.empty();
}

TimerQueue::Clock::time_point TimerQueue::nextDeadline() const {
  auto earliest = Clock::time_point::max();
  for (const auto &[timerID, timer] : timers_) earliest = std::min(earliest, timer.deadline);
  return earliest;
}

std::vector<std::uint32_t> TimerQueue::expire(Clock::time_point now) {
  std::vector<std::pair<Clock::time_point, std::uint32_t>> expired;
  for (auto entry = timers_.begin(); entry != timers_.end();) {
    auto &[timerID, timer] = *entry;
    if (timer.deadline > now) {
      ++entry;
      continue;
    }
    expired.emplace_back(timer.deadline, timerID);
    if (timer.recurring) {
      timer.deadline = now + std::max(timer.interval, kMinimumInterval);
      ++entry;
    } else {
      entry = timers_.erase(entry);
    }
  }
  std::ranges::sort(expired);
  std::vector<std::uint32_t> timerIDs;
  timerIDs.reserve(expired.size());
  for (const auto &[deadline, timerID] : expired) timerIDs.push_back(timerID);
  return timerIDs;
}

} // namespace rntizen
