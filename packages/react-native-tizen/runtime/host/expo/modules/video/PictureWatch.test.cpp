#include "PictureWatch.h"

#include <doctest/doctest.h>

namespace rntizen::expo {

namespace {

using Arrival = PictureWatch::Arrival;
using std::chrono::milliseconds;

const PictureWatch::Clock::time_point kStart{};

} // namespace

TEST_CASE("a source shows no picture until its playback has moved on") {
  PictureWatch watch;
  CHECK_FALSE(watch.shown());
  CHECK_FALSE(watch.due());
  CHECK(watch.sample(5000, kStart) == Arrival::NotYet);

  watch.expect(0, kStart);
  CHECK(watch.due());
  CHECK(watch.sample(0, kStart + milliseconds(33)) == Arrival::NotYet);
  CHECK(watch.sample(99, kStart + milliseconds(66)) == Arrival::NotYet);
  CHECK_FALSE(watch.shown());

  CHECK(watch.sample(100, kStart + milliseconds(99)) == Arrival::Moved);
  CHECK(watch.shown());
  CHECK_FALSE(watch.due());
  CHECK(watch.sample(200, kStart + milliseconds(133)) == Arrival::NotYet);
}

TEST_CASE("playback that resumes far into the media counts from where it runs") {
  PictureWatch watch;
  watch.expect(600000, kStart);
  CHECK(watch.sample(600050, kStart + milliseconds(33)) == Arrival::NotYet);
  CHECK(watch.sample(600100, kStart + milliseconds(66)) == Arrival::Moved);
}

TEST_CASE("a position that jumps back, as a loop does, counts from there") {
  PictureWatch watch;
  watch.expect(9950, kStart);
  CHECK(watch.sample(0, kStart + milliseconds(33)) == Arrival::NotYet);
  CHECK(watch.sample(99, kStart + milliseconds(66)) == Arrival::NotYet);
  CHECK(watch.sample(100, kStart + milliseconds(99)) == Arrival::Moved);
}

TEST_CASE("a position that stands still, or a player that gives none, shows after two seconds of playing") {
  PictureWatch still;
  still.expect(0, kStart);
  CHECK(still.sample(0, kStart + milliseconds(1999)) == Arrival::NotYet);
  CHECK(still.sample(0, kStart + milliseconds(2000)) == Arrival::Assumed);
  CHECK(still.shown());

  PictureWatch silent;
  silent.expect(0, kStart);
  CHECK(silent.sample(std::nullopt, kStart + milliseconds(1999)) == Arrival::NotYet);
  CHECK(silent.sample(std::nullopt, kStart + milliseconds(2000)) == Arrival::Assumed);
}

TEST_CASE("a pause or a seek before the picture puts it off until playback runs again") {
  PictureWatch watch;
  watch.expect(0, kStart);
  CHECK(watch.sample(60, kStart + milliseconds(33)) == Arrival::NotYet);

  watch.hold();
  CHECK_FALSE(watch.due());
  CHECK(watch.sample(5000, kStart + milliseconds(5000)) == Arrival::NotYet);
  CHECK_FALSE(watch.shown());

  watch.expect(30000, kStart + milliseconds(6000));
  CHECK(watch.sample(30050, kStart + milliseconds(7999)) == Arrival::NotYet);
  CHECK(watch.sample(30100, kStart + milliseconds(8000)) == Arrival::Moved);
}

TEST_CASE("buffering while the picture is due starts the two seconds again, and keeps what playback has moved") {
  PictureWatch watch;
  watch.expect(0, kStart);
  CHECK(watch.sample(60, kStart + milliseconds(60)) == Arrival::NotYet);
  watch.stalled(kStart + milliseconds(1500));
  CHECK(watch.sample(60, kStart + milliseconds(3499)) == Arrival::NotYet);
  CHECK(watch.sample(60, kStart + milliseconds(3500)) == Arrival::Assumed);

  PictureWatch moving;
  moving.expect(0, kStart);
  CHECK(moving.sample(60, kStart + milliseconds(60)) == Arrival::NotYet);
  moving.stalled(kStart + milliseconds(70));
  CHECK(moving.sample(100, kStart + milliseconds(100)) == Arrival::Moved);

  PictureWatch idle;
  idle.stalled(kStart);
  CHECK_FALSE(idle.due());
}

TEST_CASE("a picture stays through a pause and a seek, and goes with its source") {
  PictureWatch watch;
  watch.expect(0, kStart);
  REQUIRE(watch.sample(100, kStart + milliseconds(100)) == Arrival::Moved);

  watch.hold();
  CHECK(watch.shown());
  watch.expect(40000, kStart + milliseconds(200));
  CHECK(watch.shown());
  CHECK_FALSE(watch.due());

  watch.lose();
  CHECK_FALSE(watch.shown());
  CHECK_FALSE(watch.due());
  CHECK(watch.sample(50000, kStart + milliseconds(9000)) == Arrival::NotYet);

  watch.expect(0, kStart + milliseconds(9000));
  CHECK(watch.sample(100, kStart + milliseconds(9100)) == Arrival::Moved);
}

TEST_CASE("a source that goes while its picture is due leaves nothing due") {
  PictureWatch watch;
  watch.expect(0, kStart);
  watch.lose();
  CHECK_FALSE(watch.due());
  CHECK(watch.sample(100, kStart + milliseconds(3000)) == Arrival::NotYet);
  CHECK_FALSE(watch.shown());
}

} // namespace rntizen::expo
