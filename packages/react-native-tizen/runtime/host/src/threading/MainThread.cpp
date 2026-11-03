#include "MainThread.h"

#include <Ecore.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>

namespace rntizen {

namespace {

struct SyncJob {
  bool finished{false};
  bool abandoned{false};
};

struct PriorityTask {
  const std::function<void()> *task;
  std::shared_ptr<SyncJob> job;
};

struct SyncGate {
  std::mutex mutex;
  std::condition_variable changed;
  bool closing{false};
  std::deque<PriorityTask> priority;
};

SyncGate &gate() {
  static SyncGate shared;
  return shared;
}

std::atomic<bool> priorityPending{false};
std::atomic<bool> waitMeasured{false};
std::atomic<std::int64_t> waitNanos{0};
std::atomic<std::int64_t> waiters{0};
std::atomic<std::int64_t> waitersSinceNanos{0};

void runQueuedTask(void *data) {
  std::unique_ptr<std::function<void()>> task(static_cast<std::function<void()> *>(data));
  (*task)();
}

std::shared_ptr<SyncJob> openJob(SyncGate &shared) {
  std::lock_guard lock(shared.mutex);
  if (shared.closing) return nullptr;
  return std::make_shared<SyncJob>();
}

void runJob(SyncGate &shared, const std::function<void()> &task, SyncJob &job) {
  std::unique_lock lock(shared.mutex);
  if (shared.closing || job.abandoned || job.finished) return;
  lock.unlock();
  task();
  lock.lock();
  job.finished = true;
  shared.changed.notify_all();
}

bool awaitJob(SyncGate &shared, SyncJob &job) {
  const bool measured = waitMeasured.load(std::memory_order_relaxed);
  const auto started = measured ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
  if (measured) {
    waiters.fetch_add(1, std::memory_order_relaxed);
    waitersSinceNanos.fetch_add(started.time_since_epoch().count(), std::memory_order_relaxed);
  }
  std::unique_lock lock(shared.mutex);
  shared.changed.wait(lock, [&] { return job.finished || shared.closing; });
  job.abandoned = !job.finished;
  if (measured) {
    waiters.fetch_sub(1, std::memory_order_relaxed);
    waitersSinceNanos.fetch_sub(started.time_since_epoch().count(), std::memory_order_relaxed);
    waitNanos.fetch_add((std::chrono::steady_clock::now() - started).count(), std::memory_order_relaxed);
  }
  return job.finished;
}

} // namespace

void runOnMainThread(std::function<void()> task) {
  ecore_main_loop_thread_safe_call_async(runQueuedTask, new std::function<void()>(std::move(task)));
}

bool runOnMainThreadSync(const std::function<void()> &task) {
  if (isMainThread()) {
    task();
    return true;
  }
  auto &shared = gate();
  auto job = openJob(shared);
  if (job == nullptr) return false;
  runOnMainThread([job, &task, &shared] { runJob(shared, task, *job); });
  return awaitJob(shared, *job);
}

bool runOnMainThreadSoon(const std::function<void()> &task) {
  if (isMainThread()) {
    task();
    return true;
  }
  auto &shared = gate();
  auto job = openJob(shared);
  if (job == nullptr) return false;
  {
    std::lock_guard lock(shared.mutex);
    shared.priority.push_back({.task = &task, .job = job});
    priorityPending.store(true, std::memory_order_release);
  }
  runOnMainThread(runPriorityTasks);
  return awaitJob(shared, *job);
}

void runPriorityTasks() {
  if (!priorityPending.load(std::memory_order_acquire) || !isMainThread()) return;
  auto &shared = gate();
  std::deque<PriorityTask> tasks;
  {
    std::lock_guard lock(shared.mutex);
    tasks.swap(shared.priority);
    priorityPending.store(false, std::memory_order_relaxed);
  }
  for (const auto &entry : tasks) runJob(shared, *entry.task, *entry.job);
}

void closeMainThreadSync() {
  auto &shared = gate();
  std::lock_guard lock(shared.mutex);
  shared.closing = true;
  shared.priority.clear();
  priorityPending.store(false, std::memory_order_relaxed);
  shared.changed.notify_all();
}

void openMainThreadSync() {
  auto &shared = gate();
  std::lock_guard lock(shared.mutex);
  shared.closing = false;
}

bool isMainThread() {
  return eina_main_loop_is() == EINA_TRUE;
}

void setMainThreadWaitMeasured(bool measured) {
  waitMeasured.store(measured, std::memory_order_relaxed);
}

std::chrono::nanoseconds mainThreadWaitTime() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto ongoing = waiters.load(std::memory_order_relaxed) * now - waitersSinceNanos.load(std::memory_order_relaxed);
  return std::chrono::nanoseconds(waitNanos.load(std::memory_order_relaxed) + std::max<std::int64_t>(0, ongoing));
}

} // namespace rntizen
