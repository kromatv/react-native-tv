#pragma once

#include "PlayerBackend.h"
#include "PlayerLog.h"

#include "platform/MainLoop.h"
#include "threading/MainLoop.test.h"

#include <chrono>
#include <format>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace rnwebos::expo::testing {

/// Runs the main loop until `done()` or `timeout` passes.
inline bool pumpUntil(const std::function<bool()> &done, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
  const auto giveUp = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < giveUp) {
    MainLoop::get().iterate(true);
    if (done()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
}

/// Runs the main loop for `time`.
inline void pumpFor(std::chrono::milliseconds time) {
  pumpUntil([] { return false; }, time);
}

/// Writes down what a backend reports, one line per report, such as `loaded 42`.
class RecordingListener final : public PlayerBackendListener {
 public:
  void backendLoaded(const LoadedMedia &media) override {
    loaded = media;
    lines.push_back(std::format(
        "loaded {} {}x{} {}+{}",
        decimal(media.duration, 1),
        media.video.width,
        media.video.height,
        media.audioTracks.size(),
        media.textTracks.size()));
  }
  void backendPlayingChanged(bool playing) override {
    lines.push_back(playing ? "playing" : "paused");
  }
  void backendPosition(double seconds) override {
    position = seconds;
    ++positions;
  }
  void backendBuffered(double seconds) override {
    buffered = seconds;
  }
  void backendBuffering(bool buffering) override {
    lines.push_back(buffering ? "buffering" : "buffered");
  }
  void backendSeekDone() override {
    lines.push_back("seekDone");
  }
  void backendEnded() override {
    lines.push_back("ended");
  }
  void backendVideoChanged(const VideoStream &video) override {
    lines.push_back(std::format("video {}x{}", video.width, video.height));
  }
  void backendTrackChanged(TrackKind kind, int index) override {
    lines.push_back(std::format("{} track {}", kind == TrackKind::Audio ? "audio" : "text", index));
  }
  void backendFailed(const std::string &message) override {
    lines.push_back("failed: " + message);
  }

  bool has(const std::string &line) const {
    for (const auto &candidate : lines) {
      if (candidate == line) return true;
    }
    return false;
  }

  std::vector<std::string> lines;
  LoadedMedia loaded;
  double position{-1};
  double buffered{-1};
  int positions{0};
};

} // namespace rnwebos::expo::testing
