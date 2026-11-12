#include "ThreadTimerRegistry.h"

#include "TimerQueue.h"

#include <pthread.h>

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <vector>

namespace rntizen {

namespace {

using Clock = TimerQueue::Clock;

constexpr auto kFrameFallback = std::chrono::milliseconds(100);

} // namespace

struct ThreadTimerRegistry::State {
  std::mutex mutex;
  std::condition_variable wake;
  TimerQueue timers;
  std::vector<uint32_t> due;
  Clock::time_point dueSince;
  std::weak_ptr<facebook::react::TimerManager> timerManager;
  bool quitting{false};

  std::vector<uint32_t> takeDue() {
    std::vector<uint32_t> taken;
    taken.swap(due);
    return taken;
  }

  void markDue(const std::vector<uint32_t> &expired, Clock::time_point now) {
    if (expired.empty()) return;
    if (due.empty()) dueSince = now;
    for (auto timerID : expired) {
      if (std::ranges::find(due, timerID) == due.end()) due.push_back(timerID);
    }
  }

  Clock::time_point wakeAt() const {
    const auto deadline = timers.nextDeadline();
    return due.empty() ? deadline : std::min(deadline, dueSince + kFrameFallback);
  }
};

namespace {

bool fire(const std::vector<uint32_t> &timerIDs, const std::shared_ptr<facebook::react::TimerManager> &timerManager) {
  if (!timerManager) return false;
  for (auto timerID : timerIDs) timerManager->callTimer(timerID);
  return !timerIDs.empty();
}

} // namespace

bool ThreadTimerRegistry::fireDueOnFrame(State &state) {
  std::vector<uint32_t> timerIDs;
  std::shared_ptr<facebook::react::TimerManager> timerManager;
  {
    std::lock_guard lock(state.mutex);
    if (state.quitting) return false;
    timerIDs = state.takeDue();
    timerManager = state.timerManager.lock();
  }
  return fire(timerIDs, timerManager);
}

ThreadTimerRegistry::ThreadTimerRegistry()
    : state_(std::make_shared<State>()),
      frames_(std::make_shared<FrameClock>([state = state_] { return fireDueOnFrame(*state); })),
      thread_([this] { run(); }) {}

ThreadTimerRegistry::~ThreadTimerRegistry() noexcept {
  quit();
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
  const auto delay = std::chrono::microseconds(static_cast<int64_t>(std::max(0.0, delayMS) * 1000));
  {
    std::lock_guard lock(state_->mutex);
    if (state_->quitting) return;
    state_->timers.schedule(timerID, delay, recurring, Clock::now());
  }
  state_->wake.notify_one();
}

void ThreadTimerRegistry::run() {
  pthread_setname_np(pthread_self(), "rn-timers");
  auto &state = *state_;
  std::unique_lock lock(state.mutex);
  while (!state.quitting) {
    const auto wakeAt = state.wakeAt();
    if (wakeAt == Clock::time_point::max()) {
      state.wake.wait(lock, [&state] { return state.quitting || !state.timers.empty() || !state.due.empty(); });
      continue;
    }
    if (state.wake.wait_until(lock, wakeAt) != std::cv_status::timeout) continue;

    const auto now = Clock::now();
    const auto expired = state.timers.expire(now);
    state.markDue(expired, now);
    std::vector<uint32_t> late;
    if (!state.due.empty() && now - state.dueSince >= kFrameFallback) late = state.takeDue();
    auto timerManager = late.empty() ? nullptr : state.timerManager.lock();
    lock.unlock();
    if (!expired.empty()) frames_->request();
    fire(late, timerManager);
    lock.lock();
  }
}

} // namespace rntizen
