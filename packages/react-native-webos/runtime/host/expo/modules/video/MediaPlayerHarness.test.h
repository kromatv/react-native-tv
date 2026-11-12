#pragma once

#include "MediaPlayer.h"

#include "VideoTestSupport.test.h"

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rnwebos::expo::testing {

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

inline LoadedMedia media(double duration = 100, int width = 1920, int height = 1080) {
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

} // namespace rnwebos::expo::testing
