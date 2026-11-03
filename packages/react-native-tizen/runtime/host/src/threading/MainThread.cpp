#include "MainThread.h"

#include "../logging/Log.h"

#include <Ecore.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>

namespace rntizen {

namespace {

struct SyncJob {
  bool finished{false};
  bool abandoned{false};
  std::exception_ptr error;
};

struct SyncTask {
  const std::function<void()> *task;
  std::shared_ptr<SyncJob> job;
};

struct WaitStats {
  std::atomic<bool> measured{false};
  std::atomic<std::int64_t> nanos{0};
  std::atomic<std::int64_t> waiters{0};
  std::atomic<std::int64_t> waitersSinceNanos{0};
};

struct SyncGate {
  std::mutex mutex;
  std::condition_variable changed;
  bool closing{false};
  std::deque<SyncTask> priority;
  std::atomic<bool> priorityPending{false};
  WaitStats waits;
};

SyncGate &gate() {
  static SyncGate shared;
  return shared;
}

void runQueuedTask(void *data) {
  const std::unique_ptr<std::function<void()>> task(static_cast<std::function<void()> *>(data));
  try {
    (*task)();
  } catch (const std::exception &error) {
    logUncaughtException("the main loop", error);
  } catch (...) {
    logUncaughtException("the main loop");
  }
}

std::shared_ptr<SyncJob> openJob(SyncGate &shared) {
  std::lock_guard lock(shared.mutex);
  if (shared.closing) return nullptr;
  return std::make_shared<SyncJob>();
}

void runJob(SyncGate &shared, const SyncTask &entry) {
  auto &job = *entry.job;
  std::unique_lock lock(shared.mutex);
  if (shared.closing || job.abandoned || job.finished) return;
  lock.unlock();
  std::exception_ptr error;
  try {
    (*entry.task)();
  } catch (...) {
    error = std::current_exception();
  }
  lock.lock();
  job.error = error;
  job.finished = true;
  shared.changed.notify_all();
}

bool awaitJob(SyncGate &shared, SyncJob &job) {
  auto &waits = shared.waits;
  const bool measured = waits.measured.load(std::memory_order_relaxed);
  const auto started = measured ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
  if (measured) {
    waits.waiters.fetch_add(1, std::memory_order_relaxed);
    waits.waitersSinceNanos.fetch_add(started.time_since_epoch().count(), std::memory_order_relaxed);
  }
  std::unique_lock lock(shared.mutex);
  shared.changed.wait(lock, [&job, &shared] { return job.finished || shared.closing; });
  job.abandoned = !job.finished;
  if (measured) {
    waits.waiters.fetch_sub(1, std::memory_order_relaxed);
    waits.waitersSinceNanos.fetch_sub(started.time_since_epoch().count(), std::memory_order_relaxed);
    waits.nanos.fetch_add((std::chrono::steady_clock::now() - started).count(), std::memory_order_relaxed);
  }
  if (job.error) std::rethrow_exception(job.error);
  return job.finished;
}

} // namespace

void runOnMainThread(std::function<void()> task) {
  auto owned = std::make_unique<std::function<void()>>(std::move(task));
  ecore_main_loop_thread_safe_call_async(runQueuedTask, owned.release());
}

bool runOnMainThreadSync(const std::function<void()> &task) {
  if (isMainThread()) {
    task();
    return true;
  }
  auto &shared = gate();
  auto job = openJob(shared);
  if (job == nullptr) return false;
  runOnMainThread([entry = SyncTask{.task = &task, .job = job}, &shared] { runJob(shared, entry); });
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
    shared.priorityPending.store(true, std::memory_order_relaxed);
  }
  runOnMainThread(runPriorityTasks);
  return awaitJob(shared, *job);
}

void runPriorityTasks() {
  auto &shared = gate();
  if (!shared.priorityPending.load(std::memory_order_relaxed) || !isMainThread()) return;
  std::deque<SyncTask> tasks;
  {
    std::lock_guard lock(shared.mutex);
    tasks.swap(shared.priority);
    shared.priorityPending.store(false, std::memory_order_relaxed);
  }
  for (const auto &entry : tasks) runJob(shared, entry);
}

void closeMainThreadSync() {
  auto &shared = gate();
  std::lock_guard lock(shared.mutex);
  shared.closing = true;
  shared.priority.clear();
  shared.priorityPending.store(false, std::memory_order_relaxed);
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
  gate().waits.measured.store(measured, std::memory_order_relaxed);
}

std::chrono::nanoseconds mainThreadWaitTime() {
  const auto &waits = gate().waits;
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  const auto ongoing =
      waits.waiters.load(std::memory_order_relaxed) * now - waits.waitersSinceNanos.load(std::memory_order_relaxed);
  return std::chrono::nanoseconds(waits.nanos.load(std::memory_order_relaxed) + std::max<std::int64_t>(0, ongoing));
}

} // namespace rntizen
