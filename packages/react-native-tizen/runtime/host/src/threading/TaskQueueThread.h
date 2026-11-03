#pragma once

#include <cxxreact/MessageQueueThread.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace rntizen {

/// A thread that runs tasks in submission order: the JavaScript thread React
/// Native's runtime executes on. A task that throws is logged and does not stop
/// the queue.
class TaskQueueThread final : public facebook::react::MessageQueueThread {
 public:
  explicit TaskQueueThread(std::string name);
  ~TaskQueueThread() override;

  TaskQueueThread(const TaskQueueThread &) = delete;
  TaskQueueThread &operator=(const TaskQueueThread &) = delete;

  void runOnQueue(std::function<void()> &&task) override;
  void runOnQueueSync(std::function<void()> &&task) override;
  void quitSynchronous() override;

  /// Runs `work` on the calling thread while this thread waits between two of
  /// its tasks, so nothing it runs can overlap `work`. Call it from another thread.
  void runWhileParked(const std::function<void()> &work);

  bool isOnQueue() const;

  /// Starts or stops adding up how long this thread's tasks run.
  void setMeasured(bool measured);

  /// How long tasks have run while measured, in total, the running one so far included.
  std::chrono::nanoseconds busyTime() const;

 private:
  void run();

  const std::string name_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> tasks_;
  bool quitting_{false};
  std::atomic<bool> measured_{false};
  std::atomic<std::int64_t> busyNanos_{0};
  std::atomic<std::int64_t> runningSinceNanos_{0};
  std::thread thread_;
};

} // namespace rntizen
