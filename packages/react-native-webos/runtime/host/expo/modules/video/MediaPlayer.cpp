#include "MediaPlayer.h"

#include "PlayerBackends.h"
#include "PlayerLog.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace rnwebos::expo {

MediaPlayer::MediaPlayer(std::weak_ptr<PlayerObserver> observer, int number, BackendFactory makeBackend)
    : observer_(std::move(observer)), number_(number), makeBackend_(std::move(makeBackend)) {}

MediaPlayer::~MediaPlayer() {
  release();
}

void MediaPlayer::open(const std::string &uri, const SourceHeaders &headers) {
  if (released_) return;
  if (!backend_) {
    backend_ = makeBackend_ ? makeBackend_(*this, number_) : makePlayerBackend(*this, number_);
    backend_->setForeground(foreground_);
  }
  progress_.cancel();
  phase_ = Phase::Idle;
  seek_.inFlight = false;
  seek_.pending.reset();
  pendingTracks_.clear();
  askedPlaying_ = false;
  duration_ = 0;
  position_ = 0;
  buffered_ = -1;
  video_ = {};
  display_.firstFrameOwner = nullptr;
  setPlaying(false);
  source_.uri = uri;
  source_.headers = headers;
  applyDisplay();
  if (uri.empty()) {
    backend_->unload();
    setStatus(PlayerStatus::Idle);
  }
}

void MediaPlayer::prepare() {
  if (!backend_ || phase_ != Phase::Idle || source_.uri.empty()) return;
  phase_ = Phase::Loading;
  setStatus(PlayerStatus::Loading);
  backend_->load(source_);
}

void MediaPlayer::play() {
  wantsPlaying_ = true;
  pump();
}

void MediaPlayer::pause() {
  wantsPlaying_ = false;
  pump();
}

void MediaPlayer::seekTo(double seconds, unsigned generation) {
  seek_.generation = generation;
  seek_.pending = std::max(0.0, duration_ > 0 ? std::min(seconds, duration_) : seconds);
  pump();
}

void MediaPlayer::pump() {
  if (!backend_ || phase_ != Phase::Ready || seek_.inFlight) return;
  if (seek_.pending) {
    seek_.target = *std::exchange(seek_.pending, std::nullopt);
    seek_.inFlight = true;
    playerLog(number_, std::format("seek to {} s", decimal(seek_.target)));
    backend_->seek(seek_.target);
    return;
  }
  if (wantsPlaying_ == askedPlaying_) return;
  askedPlaying_ = wantsPlaying_;
  playerLog(number_, wantsPlaying_ ? "play" : "pause");
  if (wantsPlaying_) {
    backend_->play();
  } else {
    backend_->pause();
  }
}

void MediaPlayer::selectTrack(TrackKind kind, int index) {
  if (kind == TrackKind::Text && index < 0) {
    pendingTracks_.erase(kind);
    if (backend_ && phase_ == Phase::Ready) backend_->selectTrack(kind, -1);
    if (auto observer = observer_.lock()) observer->playerTrackChanged(kind, -1);
    return;
  }
  if (!backend_ || phase_ != Phase::Ready) {
    pendingTracks_[kind] = index;
    return;
  }
  playerLog(number_, std::format("select {} track {}", kind == TrackKind::Audio ? "audio" : "text", index));
  backend_->selectTrack(kind, index);
}

void MediaPlayer::retryTracks() {
  const auto pending = std::exchange(pendingTracks_, {});
  for (const auto &[kind, index] : pending) selectTrack(kind, index);
}

void MediaPlayer::setMuted(bool muted) {
  settings_.muted = muted;
  applyVolume();
}

void MediaPlayer::setVolume(double volume) {
  settings_.volume = std::clamp(volume, 0.0, 1.0);
  applyVolume();
}

void MediaPlayer::applyVolume() {
  if (backend_ && phase_ == Phase::Ready) backend_->setVolume(settings_.muted ? 0 : settings_.volume);
}

void MediaPlayer::setLooping(bool looping) {
  settings_.looping = looping;
}

void MediaPlayer::setPlaybackRate(double rate) {
  position_ = position();
  positionAt_ = Clock::now();
  settings_.rate = rate;
  if (!backend_ || phase_ != Phase::Ready) return;
  playerLog(number_, std::format("playback rate {}", decimal(rate, 2)));
  backend_->setRate(rate);
}

void MediaPlayer::setPrebuffer(double seconds) {
  source_.prebuffer = std::max(0.0, seconds);
}

void MediaPlayer::setMaxResolution(int width, int height) {
  source_.maxWidth = std::max(0, width);
  source_.maxHeight = std::max(0, height);
}

void MediaPlayer::setForeground(bool foreground) {
  foreground_ = foreground;
  if (backend_) backend_->setForeground(foreground);
}

void MediaPlayer::fail(const std::string &message) {
  playerLog(number_, message, LogLevel::Error);
  wantsPlaying_ = false;
  askedPlaying_ = false;
  seek_.inFlight = false;
  seek_.pending.reset();
  progress_.cancel();
  phase_ = Phase::Idle;
  if (backend_) backend_->unload();
  setStatus(PlayerStatus::Error, message);
  setPlaying(false);
}

void MediaPlayer::setStatus(PlayerStatus status, const std::string &error) {
  if (status == status_ && status != PlayerStatus::Error) return;
  playerLog(number_, std::format("status {} -> {}", statusName(status_), statusName(status)));
  status_ = status;
  if (auto observer = observer_.lock()) observer->playerStatusChanged(status, error);
}

void MediaPlayer::setPlaying(bool playing) {
  if (playing == playing_) return;
  position_ = position();
  positionAt_ = Clock::now();
  playing_ = playing;
  reportProgress(false);
  if (auto observer = observer_.lock()) observer->playerPlayingChanged(playing);
  notifyFirstFrame();
}

void MediaPlayer::release() {
  if (std::exchange(released_, true)) return;
  progress_.cancel();
  phase_ = Phase::Idle;
  if (!backend_) return;
  backend_->plane().hide();
  backend_.reset();
  playerLog(number_, "released");
  notifyDisplayChange();
}

} // namespace rnwebos::expo
