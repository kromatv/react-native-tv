#include "VideoPlayer.h"

namespace rnwebos::expo {

bool VideoPlayer::isPlaying() const {
  return playing();
}

bool VideoPlayer::staysActiveInBackground() const {
  const auto value = stored("staysActiveInBackground", false);
  return value.isBool() && value.getBool();
}

void VideoPlayer::restart() {
  replay();
}

void VideoPlayer::seekTo(double seconds) {
  setCurrentTime(seconds);
}

bool VideoPlayer::setSubtitlesEnabled(bool enabled) {
  int index = -1;
  {
    std::lock_guard lock(mutex_);
    const auto &tracks = snapshot_.media.textTracks;
    if (tracks.empty()) return false;
    if (enabled) index = tracks.front().index;
  }
  command([index](WebOSPlayer &webos) { webos.selectTrack(TrackKind::Text, index); });
  return true;
}

} // namespace rnwebos::expo
