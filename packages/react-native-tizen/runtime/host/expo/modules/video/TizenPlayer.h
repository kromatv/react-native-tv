#pragma once

#include "PlayerModel.h"
#include "VideoFit.h"

#include <Ecore.h>
#include <player.h>

#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

namespace rntizen::expo {

class VideoView;

using SourceHeaders = std::map<std::string, std::string, std::less<>>;

/// What a TizenPlayer reports, always on the main loop.
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

/// Where a view wants the video: its box and the window, in window pixels.
struct Presentation {
  PixelRect view;
  PixelRect window;
  ContentFit fit{ContentFit::Contain};
  /// Runs once the plane shows this owner a playing source.
  std::function<void()> onFirstFrame;
  /// Runs when hasDisplay() changes.
  std::function<void()> onDisplayChange;
  /// Receives the selected text track's lines, each shown for its duration in milliseconds.
  std::function<void(const std::string &text, unsigned long milliseconds)> onSubtitle;
};

/// One capi-media-player handle bound to the app window's overlay plane, as
/// `tizen-media-playback.md` section 6 lays out. Main thread only. The slow
/// calls (`player_create`, `player_unprepare` and the release) run on the
/// player thread, which holds the handle meanwhile; every other `player_*`
/// call, the display binding included, runs on the main loop, and every
/// callback posts its effect there. Seeks queue so one is in flight, and play
/// or pause waits for it.
class TizenPlayer final : public std::enable_shared_from_this<TizenPlayer> {
 public:
  TizenPlayer(std::weak_ptr<PlayerObserver> observer, int number);
  ~TizenPlayer();

  TizenPlayer(const TizenPlayer &) = delete;
  TizenPlayer &operator=(const TizenPlayer &) = delete;

  /// Runs `command` now, or once the player thread hands the handle back, in
  /// the order commands came.
  void perform(std::function<void(TizenPlayer &)> command);

  /// Creates the handle on first use, returns it to IDLE and sets `uri`; an
  /// empty one leaves the player idle. Of `headers`, only User-Agent and
  /// Cookie reach the player.
  void open(const std::string &uri, const SourceHeaders &headers);
  /// Prepares the source `open` set, once the player thread hands the handle back if it holds it.
  void prepare();
  void play();
  void pause();
  void seekTo(double seconds, bool accurate, unsigned generation);
  void setMuted(bool muted);
  void setVolume(double volume);
  void setLooping(bool looping);
  void setPlaybackRate(double rate);
  /// Prebuffer before playback starts, applied at the next prepare.
  void setPrebuffer(double seconds);
  void setMaxResolution(int width, int height);
  /// Selects a track by index; -1 turns subtitles off, since capi cannot deselect one.
  void selectTrack(TrackKind kind, int index);
  /// How often progress is reported; 0 still polls, slower, for the position getters.
  void setProgressInterval(double seconds);

  /// `owner` shows the video at `presentation`; the last caller owns the plane.
  void present(const VideoView *owner, const Presentation &presentation);
  void withdraw(const VideoView *owner);

  /// Whether the handle is bound to the window's overlay, so there is a plane to show.
  bool hasDisplay() const;

  /// Stops and destroys the handle; the player stays released.
  void release();

 private:
  struct CallbackTarget;
  enum class Phase { Closed, Idle, Preparing, Ready };

  struct Lending {
    bool lent{false};
    std::function<void(TizenPlayer &)> resume;
    std::deque<std::function<void(TizenPlayer &)>> deferred;
  };

  struct SeekState {
    bool inFlight{false};
    bool waitsForStart{false};
    int targetMs{0};
    std::optional<std::pair<int, bool>> pending;
    unsigned generation{0};
  };

  struct Settings {
    bool muted{false};
    double volume{1};
    bool looping{false};
    double rate{1};
    int prebufferMs{0};
    double progressInterval{0};
  };

  struct DisplayState {
    const VideoView *owner{nullptr};
    const VideoView *firstFrameOwner{nullptr};
    std::optional<Presentation> presentation;
    std::optional<PlaneGeometry> plane;
    bool bound{false};
    bool visible{false};
    bool subtitlesShown{false};
  };

  void create(std::function<void(TizenPlayer &)> then);
  void lend(std::function<player_h(player_h)> work, std::function<void(TizenPlayer &)> resume);
  void reclaim(player_h handle);
  void releaseHandle();
  static void releaseOnPlayerThread(player_h handle, std::shared_ptr<CallbackTarget> target, int number, bool preparing);
  static void unsetCallbacks(player_h handle);
  void bindDisplay();
  void setCallbacks();
  void fail(const std::string &message);
  void setStatus(PlayerStatus status, const std::string &error = {});
  void pump();
  bool issueSeek(player_state_e state);
  void retryTracks();
  void refreshPlaying();
  player_state_e state() const;
  void startProgress();
  void stopProgress();
  void reportProgress(bool tick);
  LoadedMedia readMedia();
  VideoStream readVideo();
  void applyDisplay();
  void setVisible(bool visible);
  void notifyFirstFrame();
  void notifyDisplayChange();
  void applySettings();
  void applyHeaders(const SourceHeaders &headers);

  void onPrepared();
  void onCompleted();
  void onSeekCompleted();
  void onBuffering(int percent);
  void onError(int code);
  void onInterrupted(int code);
  void onVideoStreamChanged(int width, int height, int fps, int bitrate);
  void onSubtitle(const std::string &text, unsigned long milliseconds);

  static Eina_Bool progressTick(void *data);
  static void preparedCallback(void *data);
  static void completedCallback(void *data);
  static void seekCompletedCallback(void *data);
  static void bufferingCallback(int percent, void *data);
  static void errorCallback(int code, void *data);
  static void interruptedCallback(player_interrupted_code_e code, void *data);
  static void videoStreamChangedCallback(int width, int height, int fps, int bitrate, void *data);
  static void subtitleCallback(unsigned long duration, char *text, void *data);

  const std::weak_ptr<PlayerObserver> observer_;
  const int number_;
  std::shared_ptr<CallbackTarget> target_;
  player_h handle_{nullptr};
  Phase phase_{Phase::Closed};
  bool released_{false};
  Lending lending_;
  std::string uri_;
  bool sourceSet_{false};
  PlayerStatus status_{PlayerStatus::Idle};
  bool playing_{false};
  bool wantsPlaying_{false};
  SeekState seek_;
  std::map<TrackKind, int> pendingTracks_;
  Settings settings_;
  double duration_{0};
  Ecore_Timer *progress_{nullptr};
  VideoStream video_;
  DisplayState display_;
};

} // namespace rntizen::expo
