#include "TizenPlayer.h"

#include "PlayerLog.h"

#include "HostWindow.h"
#include "threading/MainThread.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>

namespace rntizen::expo {

void TizenPlayer::bindDisplay() {
  auto *window = hostWindow();
  if (window == nullptr) {
    playerLog(number_, "no window to bind the overlay to", LogLevel::Warning);
    return;
  }
  const auto started = std::chrono::steady_clock::now();
  const int bound = player_set_display(handle_, PLAYER_DISPLAY_TYPE_OVERLAY, GET_DISPLAY(window));
  const int mode = player_set_display_mode(handle_, PLAYER_DISPLAY_MODE_DST_ROI);
  playerLog(
      number_,
      "player_set_display(OVERLAY, elm_win) " + playerErrorName(bound) + ", DST_ROI " + playerErrorName(mode) + " in " +
          millisSince(started) + ", " + threadDescription());
  displayBound_ = bound == PLAYER_ERROR_NONE;
  visible_ = true;
  setVisible(false);
  notifyDisplayChange();
}

void TizenPlayer::applySettings() {
  player_set_mute(handle_, muted_);
  player_set_volume(handle_, static_cast<float>(volume_), static_cast<float>(volume_));
  player_set_looping(handle_, looping_);
}

void TizenPlayer::open(const std::string &uri, const std::map<std::string, std::string> &headers) {
  if (released_) return;
  if (handle_ == nullptr) {
    create([uri, headers](TizenPlayer &self) { self.open(uri, headers); });
    return;
  }
  stopProgress();
  if (state() != PLAYER_STATE_IDLE || phase_ == Phase::Preparing) {
    phase_ = Phase::Idle;
    lend(
        [number = number_](player_h handle) {
          const auto started = std::chrono::steady_clock::now();
          const int unprepared = player_unprepare(handle);
          playerLog(number, "unprepare " + playerErrorName(unprepared) + " in " + millisSince(started) + ", " +
                                threadDescription());
          return handle;
        },
        [uri, headers](TizenPlayer &self) { self.open(uri, headers); });
    return;
  }
  phase_ = Phase::Idle;
  seeking_ = false;
  seekWaitsForStart_ = false;
  pendingSeek_.reset();
  pendingTracks_.clear();
  subtitlesShown_ = false;
  duration_ = 0;
  video_ = {};
  firstFrameOwner_ = nullptr;
  refreshPlaying();
  uri_ = uri;
  sourceSet_ = false;
  if (uri_.empty()) {
    setStatus(PlayerStatus::Idle);
    return;
  }
  const int set = player_set_uri(handle_, uri_.c_str());
  playerLog(number_, "player_set_uri " + playerErrorName(set) + ": " + uri_);
  if (set != PLAYER_ERROR_NONE) {
    fail("The player refused the source: " + playerErrorName(set));
    return;
  }
  sourceSet_ = true;
  applyHeaders(headers);
}

void TizenPlayer::applyHeaders(const std::map<std::string, std::string> &headers) {
  for (const auto &[name, value] : headers) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    const int size = static_cast<int>(value.size());
    if (lower == "user-agent") {
      playerLog(number_, "User-Agent " + playerErrorName(player_set_streaming_user_agent(handle_, value.c_str(), size)));
    } else if (lower == "cookie") {
      playerLog(number_, "Cookie " + playerErrorName(player_set_streaming_cookie(handle_, value.c_str(), size)));
    } else {
      playerLog(number_, "header " + name + " dropped: the player takes only User-Agent and Cookie", LogLevel::Warning);
    }
  }
}

void TizenPlayer::prepare() {
  if (lent_) {
    deferred_.push_back([](TizenPlayer &self) { self.prepare(); });
    return;
  }
  if (handle_ == nullptr || phase_ != Phase::Idle || !sourceSet_) return;
  if (prebufferMs_ > 0) {
    playerLog(
        number_,
        "prebuffer " + std::to_string(prebufferMs_) + " ms " +
            playerErrorName(player_set_streaming_buffering_time(handle_, prebufferMs_, -1)));
  }
  phase_ = Phase::Preparing;
  setStatus(PlayerStatus::Loading);
  auto result = std::make_shared<int>(PLAYER_ERROR_NONE);
  lend(
      [number = number_, target = target_.get(), result](player_h handle) {
        const auto started = std::chrono::steady_clock::now();
        *result = player_prepare_async(handle, preparedCallback, target);
        playerLog(number, "player_prepare_async " + playerErrorName(*result) + " in " + millisSince(started) + ", " +
                              threadDescription());
        return handle;
      },
      [result](TizenPlayer &self) {
        if (*result == PLAYER_ERROR_NONE || self.phase_ != Phase::Preparing) return;
        self.phase_ = Phase::Idle;
        self.fail("The player cannot open the source: " + playerErrorName(*result));
      });
}

void TizenPlayer::play() {
  wantsPlaying_ = true;
  pump();
}

void TizenPlayer::pause() {
  wantsPlaying_ = false;
  pump();
}

void TizenPlayer::seekTo(double seconds, bool accurate, unsigned generation) {
  seekGeneration_ = generation;
  const auto clamped = std::max(0.0, duration_ > 0 ? std::min(seconds, duration_) : seconds);
  pendingSeek_ = std::make_pair(static_cast<int>(std::lround(clamped * 1000)), accurate);
  pump();
}

void TizenPlayer::pump() {
  if (handle_ == nullptr || phase_ != Phase::Ready || seeking_) return;
  auto current = state();
  if (pendingSeek_ && !(seekWaitsForStart_ && current == PLAYER_STATE_READY) && issueSeek(current)) return;
  if (wantsPlaying_ && current != PLAYER_STATE_PLAYING) {
    const int started = player_start(handle_);
    playerLog(number_, "player_start " + playerErrorName(started));
    if (!visible_) player_set_display_visible(handle_, false);
  } else if (!wantsPlaying_ && current == PLAYER_STATE_PLAYING) {
    playerLog(number_, "player_pause " + playerErrorName(player_pause(handle_)));
  }
  refreshPlaying();
  current = state();
  if (current != PLAYER_STATE_READY) {
    retryTracks();
    if (pendingSeek_ && seekWaitsForStart_) issueSeek(current);
  }
}

bool TizenPlayer::issueSeek(player_state_e current) {
  const auto [milliseconds, accurate] = *pendingSeek_;
  const int sought = player_set_play_position(handle_, milliseconds, accurate, seekCompletedCallback, target_.get());
  playerLog(
      number_,
      "seek to " + std::to_string(milliseconds) + " ms in " + playerStateName(current) + ": " + playerErrorName(sought));
  if (sought == PLAYER_ERROR_NONE) {
    seeking_ = true;
    seekWaitsForStart_ = false;
    seekTargetMs_ = milliseconds;
    pendingSeek_.reset();
    return true;
  }
  if (current == PLAYER_STATE_READY && !seekWaitsForStart_) {
    seekWaitsForStart_ = true;
    playerLog(number_, "the seek waits for playback to start");
    return false;
  }
  pendingSeek_.reset();
  seekWaitsForStart_ = false;
  return false;
}

void TizenPlayer::selectTrack(TrackKind kind, int index) {
  if (kind == TrackKind::Text && index < 0) {
    subtitlesShown_ = false;
    pendingTracks_.erase(kind);
    if (presentation_ && presentation_->onSubtitle) presentation_->onSubtitle({}, 0);
    if (auto observer = observer_.lock()) observer->playerTrackChanged(kind, -1);
    return;
  }
  if (handle_ == nullptr || phase_ != Phase::Ready) {
    pendingTracks_[kind] = index;
    return;
  }
  const auto type = kind == TrackKind::Audio ? PLAYER_STREAM_TYPE_AUDIO : PLAYER_STREAM_TYPE_TEXT;
  const auto current = state();
  const int selected = player_select_track(handle_, type, index);
  playerLog(
      number_,
      std::string("select ") + (kind == TrackKind::Audio ? "audio" : "text") + " track " + std::to_string(index) +
          " in " + playerStateName(current) + ": " + playerErrorName(selected));
  if (selected == PLAYER_ERROR_NONE) {
    pendingTracks_.erase(kind);
    if (kind == TrackKind::Text) subtitlesShown_ = true;
    if (auto observer = observer_.lock()) observer->playerTrackChanged(kind, index);
  } else if (current == PLAYER_STATE_READY) {
    pendingTracks_[kind] = index;
  } else {
    pendingTracks_.erase(kind);
  }
}

void TizenPlayer::retryTracks() {
  const auto pending = pendingTracks_;
  pendingTracks_.clear();
  for (const auto &[kind, index] : pending) selectTrack(kind, index);
}

void TizenPlayer::setMuted(bool muted) {
  muted_ = muted;
  if (handle_ != nullptr) player_set_mute(handle_, muted);
}

void TizenPlayer::setVolume(double volume) {
  volume_ = std::clamp(volume, 0.0, 1.0);
  if (handle_ != nullptr) player_set_volume(handle_, static_cast<float>(volume_), static_cast<float>(volume_));
}

void TizenPlayer::setLooping(bool looping) {
  looping_ = looping;
  if (handle_ != nullptr) player_set_looping(handle_, looping);
}

void TizenPlayer::setPlaybackRate(double rate) {
  rate_ = rate;
  if (handle_ == nullptr || phase_ != Phase::Ready) return;
  playerLog(
      number_,
      "playback rate " + decimal(rate, 2) + " " + playerErrorName(player_set_playback_rate(handle_, float(rate))));
}

void TizenPlayer::setPrebuffer(double seconds) {
  prebufferMs_ = seconds > 0 ? std::max(1000, static_cast<int>(std::lround(seconds * 1000))) : 0;
}

void TizenPlayer::setMaxResolution(int width, int height) {
  if (handle_ == nullptr) return;
  playerLog(
      number_,
      "max variant " + std::to_string(width) + "x" + std::to_string(height) + " " +
          playerErrorName(player_set_max_adaptive_variant_limit(handle_, -1, width, height)));
}

void TizenPlayer::fail(const std::string &message) {
  playerLog(number_, message, LogLevel::Error);
  wantsPlaying_ = false;
  setStatus(PlayerStatus::Error, message);
  refreshPlaying();
}

void TizenPlayer::setStatus(PlayerStatus status, const std::string &error) {
  if (status == status_ && status != PlayerStatus::Error) return;
  playerLog(number_, std::string("status ") + statusName(status_) + " -> " + statusName(status));
  status_ = status;
  if (auto observer = observer_.lock()) observer->playerStatusChanged(status, error);
}

void TizenPlayer::refreshPlaying() {
  const bool playing = handle_ != nullptr && state() == PLAYER_STATE_PLAYING;
  if (playing == playing_) return;
  playing_ = playing;
  reportProgress(false);
  if (auto observer = observer_.lock()) observer->playerPlayingChanged(playing);
  notifyFirstFrame();
}

player_state_e TizenPlayer::state() const {
  player_state_e current = PLAYER_STATE_NONE;
  if (handle_ != nullptr) player_get_state(handle_, &current);
  return current;
}

} // namespace rntizen::expo
