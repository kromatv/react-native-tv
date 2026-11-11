#include "MediaPlayer.h"

#include "VideoTestSupport.test.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::pumpFor;
using testing::pumpUntil;

namespace {

using Lines = std::vector<std::string>;

class RecordingPlane final : public VideoPlane {
 public:
  explicit RecordingPlane(Lines &calls) : calls_(calls) {}

  std::string windowId() override {
    return "_Window_Id_1";
  }
  void show(Scene &, const PixelRect &source, const PixelRect &destination) override {
    calls_.push_back(std::format(
        "show {},{} {}x{} at {},{} {}x{}",
        source.x, source.y, source.width, source.height,
        destination.x, destination.y, destination.width, destination.height));
  }
  void hide() override {
    calls_.emplace_back("hide");
  }

 private:
  Lines &calls_;
};

/// A backend that writes down what it is asked and reports only what the test tells it to.
class ScriptedBackend final : public PlayerBackend {
 public:
  ScriptedBackend(PlayerBackendListener &listener, Lines &calls) : listener(listener), calls_(calls), plane_(calls) {}

  void load(const MediaSource &source) override {
    calls_.push_back(std::format("load {} max {}x{} prebuffer {}", source.uri, source.maxWidth, source.maxHeight, source.prebuffer));
  }
  void unload() override {
    calls_.emplace_back("unload");
  }
  void play() override {
    calls_.emplace_back("play");
  }
  void pause() override {
    calls_.emplace_back("pause");
  }
  void seek(double seconds) override {
    calls_.push_back("seek " + decimal(seconds, 1));
  }
  void setRate(double rate) override {
    calls_.push_back("rate " + decimal(rate, 1));
  }
  void setVolume(double volume) override {
    calls_.push_back("volume " + decimal(volume, 2));
  }
  void selectTrack(TrackKind kind, int index) override {
    calls_.push_back(std::format("{} track {}", kind == TrackKind::Audio ? "audio" : "text", index));
  }
  void setForeground(bool foreground) override {
    calls_.emplace_back(foreground ? "foreground" : "background");
  }
  VideoPlane &plane() override {
    return plane_;
  }

  PlayerBackendListener &listener;

 private:
  Lines &calls_;
  RecordingPlane plane_;
};

class RecordingObserver final : public PlayerObserver {
 public:
  void playerStatusChanged(PlayerStatus status, const std::string &error) override {
    lines.push_back(std::string("status ") + statusName(status) + (error.empty() ? "" : ": " + error));
  }
  void playerPlayingChanged(bool playing) override {
    lines.emplace_back(playing ? "playing" : "not playing");
  }
  void playerLoaded(const LoadedMedia &media) override {
    lines.push_back("loaded " + decimal(media.duration, 1));
  }
  void playerProgress(double newPosition, double newBuffered, double newDuration, unsigned generation, bool tick) override {
    position = newPosition;
    buffered = newBuffered;
    duration = newDuration;
    seekGeneration = generation;
    ticks += tick ? 1 : 0;
  }
  void playerEnded() override {
    lines.emplace_back("ended");
  }
  void playerVideoChanged(const VideoStream &video) override {
    lines.push_back(std::format("video {}x{}", video.width, video.height));
  }
  void playerTrackChanged(TrackKind kind, int index) override {
    lines.push_back(std::format("{} track {}", kind == TrackKind::Audio ? "audio" : "text", index));
  }

  Lines lines;
  double position{-1};
  double buffered{-2};
  double duration{-1};
  unsigned seekGeneration{0};
  int ticks{0};
};

LoadedMedia media(double duration = 100, int width = 1920, int height = 1080) {
  return {
      .duration = duration,
      .audioTracks = {{.index = 0, .language = "en"}},
      .textTracks = {{.index = 0, .language = "en"}},
      .audioTrack = 0,
      .textTrack = -1,
      .video = {.width = width, .height = height, .fps = 25, .bitrate = 1, .codec = "h264"},
  };
}

struct Fixture {
  rnwebos::testing::MainLoopScope mainLoop;
  Lines calls;
  ScriptedBackend *backend{nullptr};
  std::shared_ptr<RecordingObserver> observer = std::make_shared<RecordingObserver>();
  MediaPlayer player{observer, 1, [this](PlayerBackendListener &listener, int) {
                       auto made = std::make_unique<ScriptedBackend>(listener, calls);
                       backend = made.get();
                       return made;
                     }};

  void openReady(double duration = 100) {
    player.open("https://example.com/a.mp4", {});
    player.prepare();
    backend->listener.backendLoaded(media(duration));
    calls.clear();
    observer->lines.clear();
  }

  Lines takeCalls() {
    return std::exchange(calls, {});
  }
};

} // namespace

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

TEST_CASE("the plane goes where the view is, fitted to the video, and the first playing frame is told once") {
  Fixture fixture;
  Scene scene(PixelSize{1920, 1080});
  int firstFrames = 0;
  int displayChanges = 0;
  const VideoView *view = nullptr;
  const auto owner = [&view] { return reinterpret_cast<const VideoView *>(&view); };
  const auto presentation = [&](ContentFit fit) {
    return Presentation{
        .scene = &scene,
        .view = {.x = 100, .y = 100, .width = 400, .height = 400},
        .window = {.x = 0, .y = 0, .width = 1920, .height = 1080},
        .fit = fit,
        .onFirstFrame = [&firstFrames] { ++firstFrames; },
        .onDisplayChange = [&displayChanges] { ++displayChanges; },
    };
  };
  fixture.player.open("https://example.com/a.mp4", {});
  fixture.calls.clear();
  fixture.player.present(owner(), presentation(ContentFit::Contain));
  CHECK(fixture.takeCalls() == Lines{"show 0,0 0x0 at 100,100 400x400"});
  fixture.player.prepare();
  fixture.backend->listener.backendLoaded(media(100, 1600, 800));
  CHECK(fixture.takeCalls() == Lines{"load https://example.com/a.mp4 max 0x0 prebuffer 0", "show 0,0 1600x800 at 100,200 400x200"});
  fixture.player.present(owner(), presentation(ContentFit::Contain));
  CHECK(fixture.calls.empty());
  fixture.player.present(owner(), presentation(ContentFit::Cover));
  CHECK(fixture.takeCalls() == Lines{"show 400,0 800x800 at 100,100 400x400"});
  fixture.player.present(owner(), presentation(ContentFit::Fill));
  CHECK(fixture.takeCalls() == Lines{"show 0,0 1600x800 at 100,100 400x400"});
  fixture.backend->listener.backendVideoChanged({.width = 800, .height = 800, .fps = 25, .bitrate = 1, .codec = "h264"});
  CHECK(fixture.takeCalls() == Lines{"show 0,0 800x800 at 100,100 400x400"});

  CHECK(firstFrames == 0);
  fixture.player.play();
  fixture.backend->listener.backendPlayingChanged(true);
  fixture.backend->listener.backendPlayingChanged(false);
  fixture.backend->listener.backendPlayingChanged(true);
  CHECK(firstFrames == 1);

  fixture.calls.clear();
  fixture.player.withdraw(nullptr);
  CHECK(fixture.calls.empty());
  fixture.player.withdraw(owner());
  fixture.player.withdraw(owner());
  CHECK(fixture.takeCalls() == Lines{"hide"});

  fixture.player.present(owner(), presentation(ContentFit::Fill));
  CHECK(fixture.player.hasDisplay());
  fixture.calls.clear();
  fixture.player.release();
  fixture.player.release();
  CHECK(fixture.calls == Lines{"hide"});
  CHECK_FALSE(fixture.player.hasDisplay());
  CHECK(displayChanges == 1);
  fixture.player.open("https://example.com/b.mp4", {});
  fixture.player.play();
  CHECK(fixture.calls == Lines{"hide"});
}

} // namespace rnwebos::expo
