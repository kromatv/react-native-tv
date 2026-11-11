#pragma once

#include "PlayerBackend.h"
#include "PlayerTimer.h"
#include "VideoFit.h"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace rnwebos::expo {

class VideoView;

/// What a MediaPlayer reports, always on the main loop.
class PlayerObserver {
 public:
  virtual ~PlayerObserver() = default;
  virtual void playerStatusChanged(PlayerStatus status, const std::string &error) = 0;
  virtual void playerPlayingChanged(bool playing) = 0;
  virtual void playerLoaded(const LoadedMedia &media) = 0;
  /// `buffered` is -1 when the player cannot tell; `seekGeneration` is the
  /// last seek the position already reflects; `tick` is true for the timer.
  virtual void playerProgress(double position, double buffered, double duration, unsigned seekGeneration, bool tick) = 0;
  virtual void playerEnded() = 0;
  virtual void playerVideoChanged(const VideoStream &video) = 0;
  virtual void playerTrackChanged(TrackKind kind, int index) = 0;
};

/// Where a view wants the video: its box and the window, in window pixels of `scene`.
struct Presentation {
  Scene *scene{nullptr};
  PixelRect view;
  PixelRect window;
  ContentFit fit{ContentFit::Contain};
  /// Runs once the plane shows this owner a playing source.
  std::function<void()> onFirstFrame;
  /// Runs when hasDisplay() changes.
  std::function<void()> onDisplayChange;
};

/// One player over a PlayerBackend: what expo-video asks of a player that a
/// pipeline does not do by itself. It opens a source, keeps one seek in
/// flight and the newest one waiting, holds play and pause until the source
/// is ready, loops, reports progress at the interval asked for, and places
/// the backend's video plane where the view that shows it is. Main thread only.
class MediaPlayer final : public PlayerBackendListener {
 public:
  using BackendFactory = std::function<std::unique_ptr<PlayerBackend>(PlayerBackendListener &listener, int number)>;

  /// `makeBackend` runs at the first `open`; without one, the device's own backend is made.
  MediaPlayer(std::weak_ptr<PlayerObserver> observer, int number, BackendFactory makeBackend = {});
  ~MediaPlayer() override;

  MediaPlayer(const MediaPlayer &) = delete;
  MediaPlayer &operator=(const MediaPlayer &) = delete;

  /// Makes `uri` the source, replacing the one before; an empty one leaves
  /// the player idle. Of `headers`, the pipeline takes User-Agent, Cookie and Referer.
  void open(const std::string &uri, const SourceHeaders &headers);
  /// Loads the source `open` set.
  void prepare();
  void play();
  void pause();
  void seekTo(double seconds, unsigned generation);
  void setMuted(bool muted);
  void setVolume(double volume);
  void setLooping(bool looping);
  void setPlaybackRate(double rate);
  /// Prebuffer before playback starts, applied at the next load.
  void setPrebuffer(double seconds);
  /// The largest rendition an adaptive stream may pick, applied at the next load.
  void setMaxResolution(int width, int height);
  /// Selects a track by index; -1 turns subtitles off.
  void selectTrack(TrackKind kind, int index);
  /// How often progress is reported as a tick; 0 for never.
  void setProgressInterval(double seconds);
  /// Whether the app is the one the user sees.
  void setForeground(bool foreground);

  /// `owner` shows the video at `presentation`; the last caller owns the plane.
  void present(const VideoView *owner, const Presentation &presentation);
  void withdraw(const VideoView *owner);

  /// Whether the player can show a picture: false once it is released.
  bool hasDisplay() const;

  /// Unloads the source for good; the player stays released.
  void release();

  void backendLoaded(const LoadedMedia &media) override;
  void backendPlayingChanged(bool playing) override;
  void backendPosition(double seconds) override;
  void backendBuffered(double seconds) override;
  void backendBuffering(bool buffering) override;
  void backendSeekDone() override;
  void backendEnded() override;
  void backendVideoChanged(const VideoStream &video) override;
  void backendTrackChanged(TrackKind kind, int index) override;
  void backendFailed(const std::string &message) override;

 private:
  using Clock = std::chrono::steady_clock;
  enum class Phase { Idle, Loading, Ready };

  struct SeekState {
    bool inFlight{false};
    double target{0};
    std::optional<double> pending;
    unsigned generation{0};
  };

  struct Settings {
    bool muted{false};
    double volume{1};
    bool looping{false};
    double rate{1};
    double progressInterval{0};
  };

  struct DisplayState {
    const VideoView *owner{nullptr};
    const VideoView *firstFrameOwner{nullptr};
    std::optional<Presentation> presentation;
    std::optional<PlaneGeometry> plane;
    Scene *scene{nullptr};
    int videoWidth{0};
    int videoHeight{0};
  };

  void fail(const std::string &message);
  void setStatus(PlayerStatus status, const std::string &error = {});
  void setPlaying(bool playing);
  void pump();
  void retryTracks();
  void applyVolume();
  double position() const;
  void startProgress();
  void reportProgress(bool tick);
  void applyDisplay();
  void notifyFirstFrame();
  void notifyDisplayChange();

  const std::weak_ptr<PlayerObserver> observer_;
  const int number_;
  const BackendFactory makeBackend_;
  std::unique_ptr<PlayerBackend> backend_;
  Phase phase_{Phase::Idle};
  bool released_{false};
  bool foreground_{true};
  MediaSource source_;
  PlayerStatus status_{PlayerStatus::Idle};
  bool playing_{false};
  bool wantsPlaying_{false};
  bool askedPlaying_{false};
  std::optional<bool> awaitedPlaying_;
  SeekState seek_;
  std::map<TrackKind, int> pendingTracks_;
  Settings settings_;
  double duration_{0};
  double position_{0};
  Clock::time_point positionAt_{};
  double buffered_{-1};
  VideoStream video_;
  DisplayState display_;
  PlayerTimer progress_;
};

} // namespace rnwebos::expo
