#include "WorkQueue.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>

namespace rntizen::expo {

namespace {

struct Latch {
  std::mutex mutex;
  std::condition_variable changed;
  int count{0};

  void arrive() {
    std::lock_guard lock(mutex);
    ++count;
    changed.notify_all();
  }

  bool waitFor(int expected) {
    std::unique_lock lock(mutex);
    return changed.wait_for(lock, std::chrono::seconds(5), [&] { return count >= expected; });
  }
};

} // namespace

TEST_CASE("runInBackground runs tasks off the calling thread, several at once") {
  Latch started;
  Latch release;
  std::mutex mutex;
  std::set<std::thread::id> threads;
  for (int index = 0; index < 3; ++index) {
    runInBackground([&] {
      {
        std::lock_guard lock(mutex);
        threads.insert(std::this_thread::get_id());
      }
      started.arrive();
      release.waitFor(1);
    });
  }
  CHECK(started.waitFor(3));
  release.arrive();
  std::lock_guard lock(mutex);
  CHECK(threads.size() == 3);
  CHECK(threads.count(std::this_thread::get_id()) == 0);
}

TEST_CASE("runInBackground keeps running tasks after one throws") {
  Latch done;
  runInBackground([] { throw std::runtime_error("a background task failed"); });
  for (int index = 0; index < 6; ++index) runInBackground([&done] { done.arrive(); });
  CHECK(done.waitFor(6));
}

} // namespace rntizen::expo
