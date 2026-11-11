#include "WorkQueue.h"

#include "logging/Log.h"

#include <folly/Indestructible.h>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rntizen::expo {

namespace {

constexpr int kThreadCount = 3;

template <typename Task>
void runLogged(Task &task) {
  try {
    task();
  } catch (const std::exception &error) {
    log(LogLevel::Error, std::string("An Expo background task failed: ") + error.what());
  }
}

class WorkQueue final {
 public:
  WorkQueue() {
    workers_.reserve(kThreadCount);
    for (int index = 0; index < kThreadCount; ++index) {
      workers_.emplace_back([this] { run(); });
    }
  }

  void push(std::function<void()> task) {
    {
      std::lock_guard lock(mutex_);
      tasks_.push_back(std::move(task));
    }
    wake_.notify_one();
  }

 private:
  [[noreturn]] void run() {
    while (true) {
      auto task = next();
      runLogged(task);
    }
  }

  std::function<void()> next() {
    std::unique_lock lock(mutex_);
    wake_.wait(lock, [this] { return !tasks_.empty(); });
    auto task = std::move(tasks_.front());
    tasks_.pop_front();
    return task;
  }

  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> tasks_;
  std::vector<std::jthread> workers_;
};

} // namespace

void runInBackground(std::function<void()> task) {
  static folly::Indestructible<WorkQueue> queue;
  queue->push(std::move(task));
}

} // namespace rntizen::expo
