#pragma once

namespace rnwebos {

/// The calls of LG's app-switching control block, `libAcbAPI.so.1`: on webOS
/// 4.x and earlier the one way to put a `com.webos.media` pipeline's picture
/// on the video plane. webOS 5 deleted the library, so whether it loads is how
/// the host tells the two video paths apart
/// (docs/reference/webos-media-playback.md, section 2.1).
struct AcbApi {
  /// `event` is -2 for a timeout, -1 an error, 0 a cancel and 1 a completed
  /// task; it arrives on a thread of the library's own.
  using Callback = void (*)(long acb, long task, long event, long appState, long playState, const char *reply);

  long (*create)(){nullptr};
  bool (*initialize)(long acb, int playerType, const char *appId, Callback callback){nullptr};
  bool (*setSinkType)(long acb, int sink){nullptr};
  bool (*setMediaId)(long acb, const char *mediaId){nullptr};
  int (*setState)(long acb, int appState, int playState, long *task){nullptr};
  int (*setMediaVideoData)(long acb, const char *json, long *task){nullptr};
  int (*setDisplayWindow)(long acb, long x, long y, long width, long height, bool fullScreen, long *task){nullptr};
};

/// The library's calls, or null where there is no such library: webOS 5 and
/// later, and every build that is not for a TV. Loaded once and never closed.
const AcbApi *acbApi();

} // namespace rnwebos
