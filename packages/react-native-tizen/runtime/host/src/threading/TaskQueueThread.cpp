#include "TaskQueueThread.h"

#include "../logging/Log.h"

#include <pthread.h>

#include <algorithm>
#include <exception>
#include <future>

namespace rntizen {

TaskQueueThread::TaskQueueThread(std::string name) : name_(std::move(name)), thread_([this] { run(); }) {}

TaskQueueThread::~TaskQueueThread() {
  quitSynchronous();
}

void TaskQueueThread::runOnQueue(std::function<void()> &&task) {
  {
    std::lock_guard lock(mutex_);
    if (quitting_) return;
    tasks_.push_back(std::move(task));
  }
  wake_.notify_one();
}

void TaskQueueThread::runOnQueueSync(std::function<void()> &&task) {
  if (isOnQueue()) {
    task();
    return;
  }
  std::promise<void> done;
  auto finished = done.get_future();
  runOnQueue([&task, &done] {
    task();
    done.set_value();
  });
  finished.wait();
}

void TaskQueueThread::runWhileParked(const std::function<void()> &work) {
  std::promise<void> parked;
  std::promise<void> released;
  runOnQueue([&parked, release = released.get_future().share()] {
    parked.set_value();
    release.wait();
  });
  parked.get_future().wait();
  work();
  released.set_value();
}

void TaskQueueThread::quitSynchronous() {
  {
    std::lock_guard lock(mutex_);
    if (quitting_) return;
    quitting_ = true;
  }
  wake_.notify_all();
  if (thread_.joinable() && !isOnQueue()) thread_.join();
}

bool TaskQueueThread::isOnQueue() const {
  return std::this_thread::get_id() == thread_.get_id();
}

void TaskQueueThread::setMeasured(bool measured) {
  measured_.store(measured, std::memory_order_relaxed);
}

std::chrono::nanoseconds TaskQueueThread::busyTime() const {
  const auto runningSince = runningSinceNanos_.load(std::memory_order_relaxed);
  auto busy = busyNanos_.load(std::memory_order_relaxed);
  if (runningSince != 0) {
    busy += std::max<std::int64_t>(0, std::chrono::steady_clock::now().time_since_epoch().count() - runningSince);
  }
  return std::chrono::nanoseconds(busy);
}

void TaskQueueThread::run() {
  pthread_setname_np(pthread_self(), name_.substr(0, 15).c_str());
  std::unique_lock lock(mutex_);
  while (true) {
    wake_.wait(lock, [this] { return quitting_ || !tasks_.empty(); });
    if (quitting_) return;
    auto task = std::move(tasks_.front());
    tasks_.pop_front();
    lock.unlock();
    const bool measured = measured_.load(std::memory_order_relaxed);
    const auto started = measured ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
    if (measured) runningSinceNanos_.store(started.time_since_epoch().count(), std::memory_order_relaxed);
    try {
      task();
    } catch (const std::exception &error) {
      log(LogLevel::Error, std::string("Uncaught exception on ") + name_ + ": " + error.what());
    }
    if (measured) {
      runningSinceNanos_.store(0, std::memory_order_relaxed);
      busyNanos_.fetch_add((std::chrono::steady_clock::now() - started).count(), std::memory_order_relaxed);
    }
    lock.lock();
  }
}

} // namespace rntizen
