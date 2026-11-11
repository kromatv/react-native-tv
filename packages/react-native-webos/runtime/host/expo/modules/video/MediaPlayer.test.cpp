#include "MediaPlayerHarness.test.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::Fixture;
using testing::Lines;
using testing::media;
using testing::pumpFor;
using testing::pumpUntil;

TEST_CASE("a player opens its source at prepare, and is ready once the backend has loaded it") {
  Fixture fixture;
  fixture.player.prepare();
  fixture.player.setMaxResolution(1280, 720);
  fixture.player.setPrebuffer(3);
  fixture.player.open("https://example.com/a.mp4", {});
  CHECK(fixture.calls == Lines{"foreground"});
  CHECK(fixture.observer->lines.empty());
  fixture.player.prepare();
  fixture.player.prepare();
  CHECK(fixture.takeCalls() == Lines{"foreground", "load https://example.com/a.mp4 max 1280x720 prebuffer 3"});
  fixture.backend->listener.backendLoaded(media(42));
  CHECK(fixture.observer->lines == Lines{"status loading", "loaded 42.0", "status readyToPlay"});
  CHECK(fixture.calls.empty());
}

TEST_CASE("play, a seek and a track asked for before the source is ready are sent once it is") {
  Fixture fixture;
  fixture.player.open("https://example.com/a.mp4", {});
  fixture.player.prepare();
  fixture.player.play();
  fixture.player.seekTo(30, 1);
  fixture.player.selectTrack(TrackKind::Audio, 0);
  fixture.player.setMuted(true);
  fixture.player.setPlaybackRate(2);
  fixture.calls.clear();
  fixture.backend->listener.backendLoaded(media());
  CHECK(fixture.takeCalls() == Lines{"volume 0.00", "rate 2.0", "audio track 0", "seek 30.0"});
  fixture.backend->listener.backendSeekDone();
  CHECK(fixture.takeCalls() == Lines{"play"});
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.backend->listener.backendTrackChanged(TrackKind::Audio, 0);
  CHECK(fixture.observer->lines ==
        Lines{"status loading", "loaded 100.0", "status readyToPlay", "playing", "audio track 0"});
  CHECK(fixture.calls.empty());
}

TEST_CASE("one seek is in flight, the newest waits, and the position is the target meanwhile") {
  Fixture fixture;
  fixture.openReady();
  fixture.player.seekTo(10, 1);
  fixture.player.seekTo(20, 2);
  fixture.player.seekTo(250, 3);
  fixture.player.play();
  CHECK(fixture.takeCalls() == Lines{"seek 10.0"});
  fixture.backend->listener.backendPosition(1);
  fixture.backend->listener.backendSeekDone();
  CHECK(fixture.takeCalls() == Lines{"seek 100.0"});
  CHECK(fixture.observer->position == doctest::Approx(100));
  fixture.backend->listener.backendPosition(2);
  CHECK(fixture.observer->position == doctest::Approx(100));
  CHECK(fixture.observer->seekGeneration == 3);
  fixture.backend->listener.backendSeekDone();
  fixture.backend->listener.backendSeekDone();
  CHECK(fixture.takeCalls() == Lines{"play"});
  fixture.backend->listener.backendPosition(99);
  CHECK(fixture.observer->position == doctest::Approx(99));
  fixture.player.seekTo(-4, 4);
  CHECK(fixture.takeCalls() == Lines{"seek 0.0"});
}

TEST_CASE("the end of the media pauses the player, or starts it over when it loops") {
  Fixture fixture;
  fixture.openReady();
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.calls.clear();
  fixture.backend->listener.backendEnded();
  CHECK(fixture.takeCalls() == Lines{"pause"});
  CHECK(fixture.observer->lines == Lines{"playing", "not playing", "ended"});
  CHECK(fixture.observer->position == doctest::Approx(100));

  fixture.player.setLooping(true);
  fixture.player.seekTo(0, 1);
  fixture.backend->listener.backendSeekDone();
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.calls.clear();
  fixture.observer->lines.clear();
  fixture.backend->listener.backendEnded();
  CHECK(fixture.takeCalls() == Lines{"seek 0.0"});
  fixture.backend->listener.backendSeekDone();
  CHECK(fixture.takeCalls() == Lines{"play"});
  CHECK(fixture.observer->lines == Lines{"ended"});
}

TEST_CASE("buffering shows as loading, and what the backend says it plays is what the player reports") {
  Fixture fixture;
  fixture.openReady();
  fixture.backend->listener.backendBuffering(true);
  fixture.backend->listener.backendBuffering(true);
  fixture.backend->listener.backendBuffering(false);
  fixture.backend->listener.backendBuffered(55);
  fixture.backend->listener.backendPosition(5);
  CHECK(fixture.observer->lines == Lines{"status loading", "status readyToPlay"});
  CHECK(fixture.observer->buffered == doctest::Approx(55));

  fixture.backend->listener.backendVideoChanged({.width = 1280, .height = 720, .fps = 25, .bitrate = 1, .codec = "h264"});
  fixture.backend->listener.backendVideoChanged({.width = 1280, .height = 720, .fps = 25, .bitrate = 1, .codec = "h264"});
  CHECK(fixture.observer->lines == Lines{"status loading", "status readyToPlay", "video 1280x720"});
}

TEST_CASE("a pause the pipeline makes by itself is taken over, and the answer to an older request is not") {
  Fixture fixture;
  fixture.openReady();
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.backend->listener.backendPlayingChanged(false);
  CHECK(fixture.takeCalls() == Lines{"play"});
  CHECK(fixture.observer->lines == Lines{"playing", "not playing"});
  fixture.player.play();
  CHECK(fixture.takeCalls() == Lines{"play"});
  fixture.backend->listener.backendPlayingChanged(true);

  fixture.player.pause();
  fixture.player.play();
  fixture.player.pause();
  CHECK(fixture.takeCalls() == Lines{"pause", "play", "pause"});
  fixture.backend->listener.backendPlayingChanged(false);
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.backend->listener.backendPlayingChanged(false);
  CHECK(fixture.calls.empty());
  fixture.player.play();
  CHECK(fixture.takeCalls() == Lines{"play"});
}

TEST_CASE("a failure unloads the source and stops the player until another source is opened") {
  Fixture fixture;
  fixture.openReady();
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.calls.clear();
  fixture.backend->listener.backendFailed("The TV's player failed: media not found (202)");
  CHECK(fixture.takeCalls() == Lines{"unload"});
  CHECK(fixture.observer->lines ==
        Lines{"playing", "status error: The TV's player failed: media not found (202)", "not playing"});
  fixture.player.play();
  fixture.player.seekTo(3, 1);
  fixture.backend->listener.backendPosition(7);
  fixture.backend->listener.backendEnded();
  CHECK(fixture.calls.empty());

  fixture.observer->lines.clear();
  fixture.player.open("https://example.com/b.mp4", {});
  fixture.player.prepare();
  fixture.backend->listener.backendLoaded(media(10));
  CHECK(fixture.observer->lines == Lines{"status loading", "loaded 10.0", "status readyToPlay"});
  CHECK(fixture.calls == Lines{"load https://example.com/b.mp4 max 0x0 prebuffer 0", "play"});
}

TEST_CASE("a released player unloads its source, and one dropped unreleased asks nothing of its backend") {
  Lines dropped;
  {
    Fixture fixture;
    fixture.openReady();
    fixture.player.setProgressInterval(0.05);
    fixture.player.release();
    fixture.player.release();
    CHECK(fixture.calls == Lines{"unload", "hide"});

    auto observer = std::make_shared<testing::RecordingObserver>();
    MediaPlayer unreleased(observer, 2, [&dropped](PlayerBackendListener &listener, int) {
      return std::make_unique<testing::ScriptedBackend>(listener, dropped);
    });
    unreleased.open("https://example.com/a.mp4", {});
    unreleased.prepare();
    unreleased.setProgressInterval(0.05);
    dropped.clear();
  }
  CHECK(dropped.empty());
}

TEST_CASE("an empty source leaves the player idle, and a media learned late is reported again without a second ready") {
  Fixture fixture;
  fixture.openReady();
  fixture.backend->listener.backendLoaded(media(77));
  CHECK(fixture.observer->lines == Lines{"loaded 77.0"});
  fixture.player.open("", {});
  fixture.player.prepare();
  CHECK(fixture.calls == Lines{"unload"});
  CHECK(fixture.observer->lines == Lines{"loaded 77.0", "status idle"});
  fixture.backend->listener.backendLoaded(media(5));
  CHECK(fixture.observer->lines.size() == 2);
}

TEST_CASE("volume, mute, rate and tracks reach a ready backend at once, and subtitles off is reported by the player") {
  Fixture fixture;
  fixture.openReady();
  fixture.player.setVolume(0.4);
  fixture.player.setMuted(true);
  fixture.player.setMuted(false);
  fixture.player.setVolume(9);
  fixture.player.setPlaybackRate(1.5);
  fixture.player.selectTrack(TrackKind::Text, 0);
  fixture.player.selectTrack(TrackKind::Text, -1);
  fixture.player.setForeground(false);
  CHECK(fixture.calls == Lines{"volume 0.40", "volume 0.00", "volume 0.40", "volume 1.00", "rate 1.5", "text track 0",
                              "text track -1", "background"});
  CHECK(fixture.observer->lines == Lines{"text track -1"});
}

TEST_CASE("progress ticks come at the interval asked for, with the position moving while it plays") {
  Fixture fixture;
  fixture.openReady();
  fixture.player.setProgressInterval(0.05);
  fixture.backend->listener.backendPosition(10);
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  REQUIRE(pumpUntil([&] { return fixture.observer->ticks >= 3; }));
  CHECK(fixture.observer->position > 10);
  CHECK(fixture.observer->position < 12);
  CHECK(fixture.observer->duration == doctest::Approx(100));
  fixture.player.setProgressInterval(0);
  const auto ticks = fixture.observer->ticks;
  pumpFor(std::chrono::milliseconds(150));
  CHECK(fixture.observer->ticks == ticks);
}

} // namespace rnwebos::expo
