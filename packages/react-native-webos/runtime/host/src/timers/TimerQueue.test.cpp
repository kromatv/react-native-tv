#include "TimerQueue.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

namespace rntizen {

namespace {

using std::chrono::milliseconds;
using Ids = std::vector<std::uint32_t>;

const TimerQueue::Clock::time_point kStart{std::chrono::seconds(100)};

} // namespace

TEST_CASE("expire returns timers that expired together by deadline, not by id") {
  TimerQueue timers;
  timers.schedule(1, milliseconds(10), false, kStart);
  timers.schedule(2, milliseconds(5), false, kStart);

  const auto fired = timers.expire(kStart + milliseconds(20));

  CHECK(fired == Ids{2, 1});
  CHECK(timers.empty());
}

TEST_CASE("expire orders timers with the same deadline by id") {
  TimerQueue timers;
  timers.schedule(7, milliseconds(5), false, kStart);
  timers.schedule(3, milliseconds(5), false, kStart);

  CHECK(timers.expire(kStart + milliseconds(5)) == Ids{3, 7});
}

TEST_CASE("expire keeps timers whose deadline is still ahead") {
  TimerQueue timers;
  timers.schedule(1, milliseconds(5), false, kStart);
  timers.schedule(2, milliseconds(50), false, kStart);

  CHECK(timers.expire(kStart + milliseconds(10)) == Ids{1});

  CHECK(timers.nextDeadline() == kStart + milliseconds(50));
  CHECK(timers.expire(kStart + milliseconds(49)).empty());
}

TEST_CASE("a recurring timer is scheduled again from the time it expired") {
  TimerQueue timers;
  timers.schedule(4, milliseconds(16), true, kStart);

  CHECK(timers.expire(kStart + milliseconds(40)) == Ids{4});

  CHECK(timers.nextDeadline() == kStart + milliseconds(56));
}

TEST_CASE("a recurring 0 ms timer repeats every millisecond") {
  TimerQueue timers;
  timers.schedule(9, milliseconds(0), true, kStart);

  CHECK(timers.expire(kStart) == Ids{9});

  CHECK(timers.nextDeadline() == kStart + milliseconds(1));
}

TEST_CASE("scheduling an id again replaces its timer") {
  TimerQueue timers;
  timers.schedule(5, milliseconds(5), true, kStart);

  timers.schedule(5, milliseconds(30), false, kStart);

  CHECK(timers.expire(kStart + milliseconds(10)).empty());
  CHECK(timers.expire(kStart + milliseconds(30)) == Ids{5});
  CHECK(timers.empty());
}

TEST_CASE("an erased or cleared timer never expires") {
  TimerQueue timers;
  timers.schedule(1, milliseconds(1), false, kStart);
  timers.schedule(2, milliseconds(1), false, kStart);

  timers.erase(1);

  CHECK(timers.expire(kStart + milliseconds(1)) == Ids{2});
  timers.schedule(3, milliseconds(1), false, kStart);
  timers.clear();
  CHECK(timers.empty());
  CHECK(timers.nextDeadline() == TimerQueue::Clock::time_point::max());
}

} // namespace rntizen
