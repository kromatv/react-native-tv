#include "ThreadTimerRegistry.h"

#include <pthread.h>

#include <algorithm>
#include <condition_variable>
#include <map>
#include <mutex>
#include <vector>

namespace rntizen {

namespace {

using Clock = std::chrono::steady_clock;

constexpr auto kFrameFallback = std::chrono::milliseconds(100);
constexpr auto kMinimumInterval = std::chrono::milliseconds(1);

struct Timer {
  Clock::time_point deadline;
  std::chrono::microseconds interval;
  bool recurring;
};

} // namespace

struct ThreadTimerRegistry::State {
  std::mutex mutex;
  std::condition_variable wake;
  std::map<uint32_t, Timer> timers;
  std::vector<uint32_t> due;
  Clock::time_point dueSince;
  std::weak_ptr<facebook::react::TimerManager> timerManager;
  bool quitting{false};

  std::vector<uint32_t> takeDue() {
    std::vector<uint32_t> taken;
    taken.swap(due);
    return taken;
  }
};

namespace {

bool fire(const std::vector<uint32_t> &timerIDs, const std::shared_ptr<facebook::react::TimerManager> &timerManager) {
  if (!timerManager) return false;
  for (auto timerID : timerIDs) timerManager->callTimer(timerID);
  return !timerIDs.empty();
}

} // namespace

ThreadTimerRegistry::ThreadTimerRegistry()
    : state_(std::make_shared<State>()),
      frames_(std::make_shared<FrameClock>([state = state_] {
        std::unique_lock lock(state->mutex);
        if (state->quitting) return false;
        auto timerIDs = state->takeDue();
        auto timerManager = state->timerManager.lock();
        lock.unlock();
        return fire(timerIDs, timerManager);
      })),
      thread_([this] { run(); }) {}

ThreadTimerRegistry::~ThreadTimerRegistry() noexcept {
  quit();
  if (thread_.joinable()) thread_.join();
}

void ThreadTimerRegistry::createTimer(uint32_t timerID, double delayMS) {
  schedule(timerID, delayMS, false);
}

void ThreadTimerRegistry::createRecurringTimer(uint32_t timerID, double delayMS) {
  schedule(timerID, delayMS, true);
}

void ThreadTimerRegistry::deleteTimer(uint32_t timerID) {
  std::lock_guard lock(state_->mutex);
  state_->timers.erase(timerID);
  std::erase(state_->due, timerID);
}

void ThreadTimerRegistry::quit() {
  {
    std::lock_guard lock(state_->mutex);
    state_->quitting = true;
    state_->timers.clear();
    state_->due.clear();
  }
  state_->wake.notify_all();
  frames_->close();
}

void ThreadTimerRegistry::setTimerManager(std::weak_ptr<facebook::react::TimerManager> timerManager) {
  std::lock_guard lock(state_->mutex);
  state_->timerManager = std::move(timerManager);
}

void ThreadTimerRegistry::schedule(uint32_t timerID, double delayMS, bool recurring) {
  const auto interval = std::chrono::microseconds(static_cast<int64_t>(std::max(0.0, delayMS) * 1000));
  {
    std::lock_guard lock(state_->mutex);
    if (state_->quitting) return;
    state_->timers[timerID] = Timer{.deadline = Clock::now() + interval, .interval = interval, .recurring = recurring};
  }
  state_->wake.notify_one();
}

void ThreadTimerRegistry::run() {
  pthread_setname_np(pthread_self(), "rn-timers");
  auto &state = *state_;
  std::unique_lock lock(state.mutex);
  while (!state.quitting) {
    auto wakeAt = Clock::time_point::max();
    for (const auto &[timerID, timer] : state.timers) wakeAt = std::min(wakeAt, timer.deadline);
    if (!state.due.empty()) wakeAt = std::min(wakeAt, state.dueSince + kFrameFallback);
    if (wakeAt == Clock::time_point::max()) {
      state.wake.wait(lock);
      continue;
    }
    if (state.wake.wait_until(lock, wakeAt) != std::cv_status::timeout) continue;

    const auto now = Clock::now();
    bool expired = false;
    for (auto entry = state.timers.begin(); entry != state.timers.end();) {
      if (entry->second.deadline > now) {
        ++entry;
        continue;
      }
      if (state.due.empty()) state.dueSince = now;
      if (std::find(state.due.begin(), state.due.end(), entry->first) == state.due.end()) {
        state.due.push_back(entry->first);
      }
      expired = true;
      if (entry->second.recurring) {
        entry->second.deadline = now + std::max<std::chrono::microseconds>(entry->second.interval, kMinimumInterval);
        ++entry;
      } else {
        entry = state.timers.erase(entry);
      }
    }
    std::vector<uint32_t> late;
    if (!state.due.empty() && now - state.dueSince >= kFrameFallback) late = state.takeDue();
    auto timerManager = late.empty() ? nullptr : state.timerManager.lock();
    lock.unlock();
    if (expired) frames_->request();
    fire(late, timerManager);
    lock.lock();
  }
}

} // namespace rntizen
