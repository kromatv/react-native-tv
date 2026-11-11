#include "MediaPlayer.h"

#include "PlayerLog.h"

#include <algorithm>
#include <format>

namespace rnwebos::expo {

void MediaPlayer::backendLoaded(const LoadedMedia &media) {
  if (phase_ == Phase::Idle) return;
  timeline_.duration = media.duration;
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
// it, and is taken over; one that answers an older request is only reported.
void MediaPlayer::backendPlayingChanged(bool playing) {
  if (phase_ != Phase::Ready) return;
  if (awaitedPlaying_ == playing) {
    awaitedPlaying_.reset();
  } else if (!awaitedPlaying_.has_value() && playing != wantsPlaying_) {
    playerLog(number_, playing ? "the pipeline started playing by itself" : "the pipeline paused by itself");
    wantsPlaying_ = playing;
    askedPlaying_ = playing;
  }
  setPlaying(playing);
  pump();
}

void MediaPlayer::backendPosition(double seconds) {
  if (phase_ != Phase::Ready || seek_.inFlight || seek_.pending.has_value()) return;
  timeline_.position = seconds;
  timeline_.positionAt = Clock::now();
  reportProgress(false);
}

void MediaPlayer::backendBuffered(double seconds) {
  timeline_.buffered = seconds;
}

void MediaPlayer::backendBuffering(bool buffering) {
  using enum PlayerStatus;
  if (phase_ != Phase::Ready) return;
  if (buffering && status_ == ReadyToPlay) {
    timeline_.position = position();
    setStatus(Loading);
  } else if (!buffering && status_ == Loading) {
    timeline_.positionAt = Clock::now();
    setStatus(ReadyToPlay);
  }
}

void MediaPlayer::backendSeekDone() {
  if (!std::exchange(seek_.inFlight, false)) return;
  timeline_.position = seek_.target;
  timeline_.positionAt = Clock::now();
  playerLog(number_, std::format("seek done at {} s", decimal(seek_.target)));
  reportProgress(false);
  pump();
}

void MediaPlayer::backendEnded() {
  if (phase_ != Phase::Ready) return;
  playerLog(number_, settings_.looping ? "ended, looping" : "ended");
  if (timeline_.duration > 0) timeline_.position = timeline_.duration;
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
  if (seek_.pending.has_value()) return *seek_.pending;
  if (seek_.inFlight) return seek_.target;
  auto position = timeline_.position;
  if (playing_ && status_ == PlayerStatus::ReadyToPlay) {
    position += std::chrono::duration<double>(Clock::now() - timeline_.positionAt).count() * settings_.rate;
  }
  return timeline_.duration > 0 ? std::min(position, timeline_.duration) : position;
}

void MediaPlayer::reportProgress(bool tick) const {
  if (phase_ != Phase::Ready) return;
  if (auto observer = observer_.lock()) {
    observer->playerProgress(position(), timeline_.buffered, timeline_.duration, seek_.generation, tick);
  }
}

} // namespace rnwebos::expo
