#pragma once

#include "PlayerModel.h"

#include <folly/dynamic.h>

#include <string>

namespace rnwebos::expo {

/// One message of a `luna://com.webos.media/subscribe` subscription, read
/// into what the player needs of it (docs/reference/webos-media-playback.md,
/// section 4). The fields beside `kind` and `name` are set for the kinds
/// their comments name.
struct LunaMediaEvent {
  enum class Kind {
    LoadCompleted,
    SourceInfo,
    VideoInfo,
    CurrentTime,
    BufferRange,
    BufferingStart,
    BufferingEnd,
    Playing,
    Paused,
    SeekDone,
    EndOfStream,
    TrackSelected,
    Error,
    UnloadCompleted,
    /// The answer to the subscribe call itself.
    Reply,
    /// An event the player has no use for, or does not know.
    Other,
  };

  Kind kind{Kind::Other};
  /// The event's key, such as `currentTime`.
  std::string name;
  /// SourceInfo: duration, tracks and the first video track.
  LoadedMedia media;
  /// SourceInfo: no duration and no track, which the pipeline sends before the real one.
  bool placeholder{false};
  /// VideoInfo.
  VideoStream video;
  /// CurrentTime: the position. BufferRange: the end of the buffered range.
  double seconds{0};
  /// BufferRange: the whole media is buffered.
  bool complete{false};
  /// TrackSelected.
  TrackKind trackKind{TrackKind::Audio};
  int trackIndex{-1};
  /// Error, and a Reply that refuses.
  int errorCode{0};
  std::string errorText;
  /// Reply.
  bool accepted{true};
};

LunaMediaEvent parseLunaMediaEvent(const folly::dynamic &message);

/// What an error of the TV's player means, as `media not found (202): <text>`.
std::string describeMediaError(int code, const std::string &text);

} // namespace rnwebos::expo
