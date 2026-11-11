#pragma once

#include <memory>

namespace rnwebos {

/// A player the host can drive without JavaScript, as when the app leaves the
/// foreground. Every method may be called from any thread.
class MediaSession {
 public:
  virtual ~MediaSession();

  virtual bool isPlaying() const = 0;
  /// expo-video's `staysActiveInBackground`.
  virtual bool staysActiveInBackground() const = 0;
  virtual void play() = 0;
  virtual void pause() = 0;
  virtual void restart() = 0;
  virtual void seekBy(double seconds) = 0;
  virtual void seekTo(double seconds) = 0;
  /// Shows the first subtitle track, or none; false when the media has none to show.
  virtual bool setSubtitlesEnabled(bool enabled) = 0;
};

/// Makes `session` known to the TV until it is destroyed.
void addMediaSession(const std::shared_ptr<MediaSession> &session);

/// Marks `session` as the one the user last played.
void markMediaSessionActive(const MediaSession *session);

/// The session a command from outside the app is for: the one playing, else the one last
/// played, else the one created last.
std::shared_ptr<MediaSession> currentMediaSession();

/// Pauses every playing session that may not stay active in the background, as
/// expo-video does on Android when the app is backgrounded, and logs the ones
/// that keep playing. `mediaCategoryDeclared` is whether the app declared that
/// it plays while hidden.
void pauseMediaSessionsForBackground(bool mediaCategoryDeclared);

} // namespace rnwebos
