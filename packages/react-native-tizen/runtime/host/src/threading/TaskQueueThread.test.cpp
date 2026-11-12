#include "TaskQueueThread.h"

#include <doctest/doctest.h>

#include <atomic>
#include <future>
#include <stdexcept>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;

} // namespace

TEST_CASE("tasks run on the queue's thread in the order they were queued") {
  TaskQueueThread queue("test-order");
  std::vector<int> order;
  std::thread::id ranOn;

  queue.runOnQueue([&order] { order.push_back(1); });
  queue.runOnQueue([&order] { order.push_back(2); });
  queue.runOnQueueSync([&order, &ranOn, &queue] {
    order.push_back(3);
    ranOn = std::this_thread::get_id();
    CHECK(queue.isOnQueue());
  });

  CHECK(order == std::vector<int>{1, 2, 3});
  CHECK(ranOn != std::this_thread::get_id());
  CHECK_FALSE(queue.isOnQueue());
}

TEST_CASE("runOnQueueSync on the queue's own thread runs the task at once") {
  TaskQueueThread queue("test-inline");
  std::vector<int> order;

  queue.runOnQueueSync([&order, &queue] {
    queue.runOnQueueSync([&order] { order.push_back(1); });
    order.push_back(2);
  });

  CHECK(order == std::vector<int>{1, 2});
}

TEST_CASE("runOnQueueSync rethrows what the task throws and the queue keeps running") {
  TaskQueueThread queue("test-throw");
  bool ranAfter = false;

  CHECK_THROWS_AS(queue.runOnQueueSync([] { throw std::runtime_error("boom"); }), std::runtime_error);

  queue.runOnQueueSync([&ranAfter] { ranAfter = true; });
  CHECK(ranAfter);
}

TEST_CASE("a task that throws is logged and does not stop the queue") {
  TaskQueueThread queue("test-log");
  bool ranAfter = false;

  queue.runOnQueue([] { throw std::runtime_error("boom"); });
  queue.runOnQueue([] { throw 42; });

  queue.runOnQueueSync([&ranAfter] { ranAfter = true; });
  CHECK(ranAfter);
}

TEST_CASE("runOnQueueSync returns without running the task once the queue has quit") {
  TaskQueueThread queue("test-quit");
  queue.quitSynchronous();
  bool ran = false;

  queue.runOnQueueSync([&ran] { ran = true; });

  CHECK_FALSE(ran);
}

TEST_CASE("a runOnQueueSync caller whose task is dropped at quit returns") {
  TaskQueueThread queue("test-drop");
  std::promise<void> gate;
  auto opened = gate.get_future().share();
  std::atomic<bool> ran{false};
  queue.runOnQueue([&queue, opened] {
    opened.wait();
    queue.quitSynchronous();
  });

  std::thread caller([&queue, &ran] { queue.runOnQueueSync([&ran] { ran = true; }); });
  std::this_thread::sleep_for(20ms);
  gate.set_value();
  caller.join();

  CHECK_FALSE(ran);
}

TEST_CASE("runWhileParked runs work while the queue waits between two tasks") {
  TaskQueueThread queue("test-park");
  std::atomic<bool> working{false};
  std::atomic<bool> overlapped{false};
  queue.runOnQueue([] { std::this_thread::sleep_for(5ms); });

  const bool ran = queue.runWhileParked([&queue, &working, &overlapped] {
    working = true;
    queue.runOnQueue([&working, &overlapped] { overlapped = working.load(); });
    std::this_thread::sleep_for(20ms);
    working = false;
  });

  queue.runOnQueueSync([] {});
  CHECK(ran);
  CHECK_FALSE(overlapped);
}

TEST_CASE("runWhileParked releases the queue when work throws") {
  TaskQueueThread queue("test-park-throw");
  bool ranAfter = false;

  CHECK_THROWS_AS(queue.runWhileParked([] { throw std::runtime_error("boom"); }), std::runtime_error);

  queue.runOnQueueSync([&ranAfter] { ranAfter = true; });
  CHECK(ranAfter);
}

TEST_CASE("runWhileParked returns false when the queue quits before it parks") {
  TaskQueueThread queue("test-park-drop");
  std::promise<void> gate;
  auto opened = gate.get_future().share();
  std::atomic<bool> ran{false};
  std::atomic<bool> parked{true};
  queue.runOnQueue([&queue, opened] {
    opened.wait();
    queue.quitSynchronous();
  });

  std::thread caller([&queue, &ran, &parked] { parked = queue.runWhileParked([&ran] { ran = true; }); });
  std::this_thread::sleep_for(20ms);
  gate.set_value();
  caller.join();

  CHECK_FALSE(parked);
  CHECK_FALSE(ran);
}

TEST_CASE("runWhileParked returns false without running work once the queue has quit") {
  TaskQueueThread queue("test-park-quit");
  queue.quitSynchronous();
  bool ran = false;

  CHECK_FALSE(queue.runWhileParked([&ran] { ran = true; }));

  CHECK_FALSE(ran);
}

TEST_CASE("busyTime adds up how long tasks ran while measured") {
  TaskQueueThread queue("test-busy");
  queue.runOnQueueSync([] { std::this_thread::sleep_for(10ms); });
  CHECK(queue.busyTime() == std::chrono::nanoseconds(0));

  queue.setMeasured(true);
  queue.runOnQueueSync([] { std::this_thread::sleep_for(10ms); });

  CHECK(queue.busyTime() >= 10ms);
}

} // namespace rntizen
