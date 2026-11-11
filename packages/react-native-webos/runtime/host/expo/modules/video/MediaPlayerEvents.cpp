#include "MediaPlayer.h"

#include "PlayerLog.h"

#include <algorithm>
#include <format>

namespace rnwebos::expo {

void MediaPlayer::backendLoaded(const LoadedMedia &media) {
  if (phase_ == Phase::Idle) return;
  duration_ = media.duration;
  video_ = media.video;
  playerLog(
      number_,
      std::format(
          "loaded: {}x{} {}, {} s, {} audio and {} text tracks",
          video_.width,
          video_.height,
          video_.codec,
          decimal(media.duration),
          media.audioTracks.size(),
          media.textTracks.size()));
  if (auto observer = observer_.lock()) observer->playerLoaded(media);
  applyDisplay();
  if (std::exchange(phase_, Phase::Ready) == Phase::Ready) return;
  setStatus(PlayerStatus::ReadyToPlay);
  if (settings_.muted || settings_.volume != 1) applyVolume();
  if (settings_.rate != 1) backend_->setRate(settings_.rate);
  startProgress();
  retryTracks();
  pump();
}

// A state nobody asked for is the pipeline's own doing, as when the TV pauses
// it: the player takes it over rather than ask again. One that only differs
// from the newest request answers an older one, and the newest still follows.
void MediaPlayer::backendPlayingChanged(bool playing) {
  if (phase_ != Phase::Ready) return;
  if (awaitedPlaying_ == playing) {
    awaitedPlaying_.reset();
  } else if (!awaitedPlaying_ && playing != wantsPlaying_) {
    playerLog(number_, playing ? "the pipeline started playing by itself" : "the pipeline paused by itself");
    wantsPlaying_ = playing;
    askedPlaying_ = playing;
  }
  setPlaying(playing);
  pump();
}

void MediaPlayer::backendPosition(double seconds) {
  if (phase_ != Phase::Ready || seek_.inFlight || seek_.pending) return;
  position_ = seconds;
  positionAt_ = Clock::now();
  reportProgress(false);
}

void MediaPlayer::backendBuffered(double seconds) {
  buffered_ = seconds;
}

void MediaPlayer::backendBuffering(bool buffering) {
  using enum PlayerStatus;
  if (phase_ != Phase::Ready) return;
  if (buffering && status_ == ReadyToPlay) {
    position_ = position();
    setStatus(Loading);
  } else if (!buffering && status_ == Loading) {
    positionAt_ = Clock::now();
    setStatus(ReadyToPlay);
  }
}

void MediaPlayer::backendSeekDone() {
  if (!std::exchange(seek_.inFlight, false)) return;
  position_ = seek_.target;
  positionAt_ = Clock::now();
  playerLog(number_, std::format("seek done at {} s", decimal(seek_.target)));
  reportProgress(false);
  pump();
}

void MediaPlayer::backendEnded() {
  if (phase_ != Phase::Ready) return;
  playerLog(number_, settings_.looping ? "ended, looping" : "ended");
  if (duration_ > 0) position_ = duration_;
  askedPlaying_ = false;
  if (settings_.looping) {
    seek_.pending = 0;
  } else {
    wantsPlaying_ = false;
    awaitedPlaying_.reset();
    setPlaying(false);
    // Paused so that a replay starts from a state every release answers `play` in.
    backend_->pause();
  }
  if (auto observer = observer_.lock()) observer->playerEnded();
  pump();
}

void MediaPlayer::backendVideoChanged(const VideoStream &video) {
  if (video == video_) return;
  video_ = video;
  playerLog(number_, std::format("video stream {}x{} at {} fps, {} b/s", video.width, video.height, video.fps, video.bitrate));
  if (auto observer = observer_.lock()) observer->playerVideoChanged(video_);
  applyDisplay();
}

void MediaPlayer::backendTrackChanged(TrackKind kind, int index) {
  if (auto observer = observer_.lock()) observer->playerTrackChanged(kind, index);
}

void MediaPlayer::backendFailed(const std::string &message) {
  fail(message);
}

void MediaPlayer::setProgressInterval(double seconds) {
  settings_.progressInterval = std::max(0.0, seconds);
  progress_.cancel();
  startProgress();
}

void MediaPlayer::startProgress() {
  if (phase_ != Phase::Ready || settings_.progressInterval <= 0) return;
  const auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::duration<double>(settings_.progressInterval));
  progress_.start(std::max(interval, std::chrono::milliseconds(16)), [this] {
    reportProgress(true);
    startProgress();
  });
}

double MediaPlayer::position() const {
  if (seek_.pending) return *seek_.pending;
  if (seek_.inFlight) return seek_.target;
  auto position = position_;
  if (playing_ && status_ == PlayerStatus::ReadyToPlay) {
    position += std::chrono::duration<double>(Clock::now() - positionAt_).count() * settings_.rate;
  }
  return duration_ > 0 ? std::min(position, duration_) : position;
}

void MediaPlayer::reportProgress(bool tick) {
  if (phase_ != Phase::Ready) return;
  if (auto observer = observer_.lock()) {
    observer->playerProgress(position(), buffered_, duration_, seek_.generation, tick);
  }
}

} // namespace rnwebos::expo
