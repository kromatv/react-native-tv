#include "SimulatedPlayer.h"

#include "VideoTestSupport.test.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::pumpFor;
using testing::pumpUntil;
using testing::RecordingListener;

namespace {

struct Fixture {
  rnwebos::testing::MainLoopScope mainLoop;
  RecordingListener listener;
  SimulatedPlayer player{listener, 1};

  bool load(const char *uri) {
    player.load({.uri = uri, .headers = {}});
    return pumpUntil([this] { return !listener.lines.empty(); });
  }
};

} // namespace

TEST_CASE("the simulated player loads the stream its URI describes, after the load time") {
  Fixture fixture;
  fixture.player.load({.uri = "https://example.com/a.mp4#sim:duration=42&load=0.05&size=1280x720", .headers = {}});
  CHECK(fixture.listener.lines.empty());
  REQUIRE(pumpUntil([&] { return !fixture.listener.lines.empty(); }));
  CHECK(fixture.listener.lines == std::vector<std::string>{"loaded 42.0 1280x720 2+1"});
  CHECK(fixture.listener.buffered == doctest::Approx(10));
}

TEST_CASE("playing moves the position at the rate asked for, and pausing stops it") {
  Fixture fixture;
  REQUIRE(fixture.load("x#sim:load=0"));
  fixture.player.setRate(4);
  fixture.player.play();
  REQUIRE(pumpUntil([&] { return fixture.listener.position >= 2; }));
  CHECK(fixture.listener.has("playing"));
  CHECK(fixture.listener.buffered == doctest::Approx(fixture.listener.position + 10));
  fixture.player.pause();
  REQUIRE(pumpUntil([&] { return fixture.listener.has("paused"); }));
  const auto stopped = fixture.listener.positions;
  pumpFor(std::chrono::milliseconds(450));
  CHECK(fixture.listener.positions == stopped);
}

TEST_CASE("a seek buffers, lands on the position clamped to the stream, and reports it") {
  Fixture fixture;
  REQUIRE(fixture.load("x#sim:load=0&duration=30"));
  fixture.player.seek(12);
  REQUIRE(pumpUntil([&] { return fixture.listener.has("buffered"); }));
  CHECK(fixture.listener.lines ==
        std::vector<std::string>{"loaded 30.0 1920x1080 2+1", "buffering", "seekDone", "buffered"});
  CHECK(fixture.listener.position == doctest::Approx(12));
  fixture.player.seek(99);
  REQUIRE(pumpUntil([&] { return fixture.listener.position == doctest::Approx(30); }));
}

TEST_CASE("playback ends at the duration and stops there, and plays again after a seek back") {
  Fixture fixture;
  REQUIRE(fixture.load("x#sim:load=0&duration=0.5"));
  fixture.player.play();
  REQUIRE(pumpUntil([&] { return fixture.listener.has("ended"); }));
  CHECK(fixture.listener.position == doctest::Approx(0.5));
  CHECK(fixture.listener.buffered == doctest::Approx(0.5));
  const auto ended = fixture.listener.positions;
  pumpFor(std::chrono::milliseconds(450));
  CHECK(fixture.listener.positions == ended);

  fixture.listener.lines.clear();
  fixture.player.seek(0);
  REQUIRE(pumpUntil([&] { return fixture.listener.has("seekDone"); }));
  fixture.player.play();
  REQUIRE(pumpUntil([&] { return fixture.listener.has("ended"); }));
  CHECK(fixture.listener.has("playing"));
}

TEST_CASE("a stream with an error fails its load, or fails at the position it names") {
  Fixture broken;
  REQUIRE(broken.load("x#sim:load=0&error=202"));
  CHECK(broken.listener.lines ==
        std::vector<std::string>{"failed: The TV cannot open the source: media not found (202): simulated"});
  broken.player.play();
  pumpFor(std::chrono::milliseconds(50));
  CHECK(broken.listener.lines.size() == 1);

  Fixture dying;
  REQUIRE(dying.load("x#sim:load=0&error-at=0.2"));
  dying.player.play();
  REQUIRE(pumpUntil([&] { return dying.listener.lines.size() == 3; }));
  CHECK(dying.listener.lines.back() == "failed: The TV's player failed: network error (300): simulated");
}

TEST_CASE("selecting a track is confirmed, turning subtitles off is not, and an unload forgets the stream") {
  Fixture fixture;
  REQUIRE(fixture.load("x#sim:load=0"));
  fixture.player.selectTrack(TrackKind::Audio, 1);
  fixture.player.selectTrack(TrackKind::Text, 0);
  fixture.player.selectTrack(TrackKind::Text, -1);
  REQUIRE(pumpUntil([&] { return fixture.listener.has("text track 0"); }));
  CHECK(fixture.listener.lines == std::vector<std::string>{"loaded 60.0 1920x1080 2+1", "audio track 1", "text track 0"});

  fixture.player.play();
  fixture.player.unload();
  fixture.player.seek(3);
  fixture.player.setVolume(0.5);
  fixture.player.setForeground(false);
  pumpFor(std::chrono::milliseconds(300));
  CHECK(fixture.listener.lines.size() == 3);
  CHECK(fixture.listener.positions == 0);
}

TEST_CASE("a load in place of another reports only the new stream") {
  Fixture fixture;
  fixture.player.load({.uri = "x#sim:load=0.05&duration=10", .headers = {}});
  fixture.player.load({.uri = "y#sim:load=0.05&duration=20", .headers = {}});
  REQUIRE(pumpUntil([&] { return !fixture.listener.lines.empty(); }));
  pumpFor(std::chrono::milliseconds(100));
  CHECK(fixture.listener.lines == std::vector<std::string>{"loaded 20.0 1920x1080 2+1"});
}

} // namespace rnwebos::expo
