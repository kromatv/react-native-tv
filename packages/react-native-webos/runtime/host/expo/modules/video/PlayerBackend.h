#pragma once

#include "PlayerModel.h"
#include "VideoPlane.h"

#include <map>
#include <string>

namespace rnwebos::expo {

using SourceHeaders = std::map<std::string, std::string, std::less<>>;

/// What a backend is asked to play. The limits apply to this load only: a
/// pipeline takes them when it opens the source.
struct MediaSource {
  std::string uri;
  SourceHeaders headers;
  /// Seconds buffered before playback starts; 0 leaves it to the pipeline.
  double prebuffer{0};
  /// The largest rendition an adaptive stream may pick; 0 for no cap.
  int maxWidth{0};
  int maxHeight{0};
};

/// What a PlayerBackend reports, always on the main thread and never from
/// inside one of its own methods.
class PlayerBackendListener {
 public:
  virtual ~PlayerBackendListener() = default;
  /// The source is open and can play. Comes again when the pipeline learns
  /// more about the media after that.
  virtual void backendLoaded(const LoadedMedia &media) = 0;
  virtual void backendPlayingChanged(bool playing) = 0;
  virtual void backendPosition(double seconds) = 0;
  /// The end of the buffered range, in seconds.
  virtual void backendBuffered(double seconds) = 0;
  /// Playback stalls for data, or goes on.
  virtual void backendBuffering(bool buffering) = 0;
  virtual void backendSeekDone() = 0;
  virtual void backendEnded() = 0;
  virtual void backendVideoChanged(const VideoStream &video) = 0;
  virtual void backendTrackChanged(TrackKind kind, int index) = 0;
  /// The source cannot play, or stopped for good.
  virtual void backendFailed(const std::string &message) = 0;
};

/// One media pipeline that plays a URI on a video plane: the TV's
/// (LunaMediaPlayer) or the simulator's clock (SimulatedPlayer). Every call
/// returns at once and its effect is reported to the listener. Main thread only.
class PlayerBackend {
 public:
  virtual ~PlayerBackend() = default;

  /// Opens `source` in place of whatever is loaded; `backendLoaded` or `backendFailed` follows.
  virtual void load(const MediaSource &source) = 0;
  virtual void unload() = 0;
  virtual void play() = 0;
  virtual void pause() = 0;
  /// Moves to `seconds`; `backendSeekDone` follows.
  virtual void seek(double seconds) = 0;
  virtual void setRate(double rate) = 0;
  /// 0 to 1; a muted player asks for 0.
  virtual void setVolume(double volume) = 0;
  /// Selects a track by index; -1 turns subtitles off.
  virtual void selectTrack(TrackKind kind, int index) = 0;
  /// Tells the pipeline whether the app is the one the user sees.
  virtual void setForeground(bool foreground) = 0;

  virtual VideoPlane &plane() = 0;
};

} // namespace rnwebos::expo
