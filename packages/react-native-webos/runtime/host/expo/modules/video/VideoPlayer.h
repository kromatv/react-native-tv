#pragma once

#include "TizenPlayer.h"

#include "../../Context.h"
#include "../../SharedObject.h"

#include "tv/MediaSessions.h"

#include <folly/dynamic.h>

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>

namespace rntizen::expo {

/// expo-video's `VideoPlayer`: the JavaScript object's native half. Getters
/// read a snapshot the main loop keeps current, setters update it and queue
/// the call for the TizenPlayer, so JavaScript never waits for the player. It
/// is also the TV's media session, which voice commands and the app going to
/// the background drive.
class VideoPlayer final : public SharedObject, public PlayerObserver, public MediaSession {
 public:
  explicit VideoPlayer(std::weak_ptr<Context> context);
  ~VideoPlayer() override;

  /// Creates the TizenPlayer and opens `source`. Call once, on the JavaScript
  /// thread, before JavaScript holds the object.
  void open(const folly::dynamic &source);

  void play() override;
  void pause() override;
  void replace(const folly::dynamic &source);
  void seekBy(double seconds) override;
  void replay();

  bool isPlaying() const override;
  bool staysActiveInBackground() const override;
  void restart() override;
  void seekTo(double seconds) override;
  bool setSubtitlesEnabled(bool enabled) override;

  bool playing() const;
  PlayerStatus status() const;
  double currentTime() const;
  void setCurrentTime(double seconds);
  double duration() const;
  double bufferedPosition() const;
  bool muted() const;
  void setMuted(bool muted);
  double volume() const;
  void setVolume(double volume);
  bool loop() const;
  void setLoop(bool loop);
  double playbackRate() const;
  void setPlaybackRate(double rate);
  double timeUpdateEventInterval() const;
  void setTimeUpdateEventInterval(double seconds);
  folly::dynamic bufferOptions() const;
  void setBufferOptions(const folly::dynamic &options);
  folly::dynamic seekTolerance() const;
  void setSeekTolerance(const folly::dynamic &tolerance);
  folly::dynamic maxResolution() const;
  void setMaxResolution(const folly::dynamic &size);
  folly::dynamic audioTrack() const;
  void setAudioTrack(const folly::dynamic &track);
  folly::dynamic subtitleTrack() const;
  void setSubtitleTrack(const folly::dynamic &track);
  folly::dynamic availableAudioTracks() const;
  folly::dynamic availableSubtitleTracks() const;
  folly::dynamic videoTrack() const;
  folly::dynamic availableVideoTracks() const;

  /// A property this platform has no player feature for: the last value JavaScript set, or `fallback`.
  folly::dynamic stored(const std::string &name, const folly::dynamic &fallback) const;
  void store(const std::string &name, const folly::dynamic &value);

  /// The player a view shows; main thread only.
  const std::shared_ptr<TizenPlayer> &tizenPlayer() const;
  int number() const;

  void sharedObjectDidRelease() override;

  void playerStatusChanged(PlayerStatus status, const std::string &error) override;
  void playerPlayingChanged(bool playing) override;
  void playerLoaded(const LoadedMedia &media) override;
  void playerProgress(double position, double buffered, double duration, unsigned seekGeneration, bool tick)
      override;
  void playerEnded() override;
  void playerVideoChanged(const VideoStream &video) override;
  void playerTrackChanged(TrackKind kind, int index) override;

 private:
  struct Snapshot {
    PlayerStatus status{PlayerStatus::Idle};
    bool playing{false};
    double position{0};
    std::chrono::steady_clock::time_point positionAt{};
    double duration{0};
    double buffered{-1};
    bool muted{false};
    double volume{1};
    bool loop{false};
    double rate{1};
    double interval{0};
    unsigned seekGeneration{0};
    folly::dynamic source{nullptr};
    folly::dynamic bufferOptions{folly::dynamic::object()};
    folly::dynamic seekTolerance{folly::dynamic::object()};
    folly::dynamic maxResolution{nullptr};
    LoadedMedia media;
    folly::dynamic stored{folly::dynamic::object()};
  };

  void command(std::function<void(TizenPlayer &)> body);
  void emit(const std::string &event, folly::dynamic payload) const;
  void openOnPlayer(const folly::dynamic &source, bool prepareNow);
  double positionLocked() const;
  bool accurateSeeks() const;

  const std::weak_ptr<Context> context_;
  const int number_;
  std::shared_ptr<TizenPlayer> player_;
  mutable std::mutex mutex_;
  Snapshot snapshot_;
};

} // namespace rntizen::expo
