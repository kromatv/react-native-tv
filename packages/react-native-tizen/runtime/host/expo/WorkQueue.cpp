#include "WorkQueue.h"

#include "logging/Log.h"

#include <condition_variable>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace rntizen::expo {

namespace {

constexpr int kThreadCount = 3;

class WorkQueue final {
 public:
  WorkQueue() {
    for (int index = 0; index < kThreadCount; ++index) {
      std::thread([this] { run(); }).detach();
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
  void run() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock lock(mutex_);
        wake_.wait(lock, [this] { return !tasks_.empty(); });
        task = std::move(tasks_.front());
        tasks_.pop_front();
      }
      try {
        task();
      } catch (const std::exception &error) {
        log(LogLevel::Error, std::string("An Expo background task failed: ") + error.what());
      }
    }
  }

  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> tasks_;
};

} // namespace

void runInBackground(std::function<void()> task) {
  static auto *queue = new WorkQueue();
  queue->push(std::move(task));
}

} // namespace rntizen::expo
