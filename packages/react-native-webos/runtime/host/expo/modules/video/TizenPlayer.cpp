#include "TizenPlayer.h"

#include "PlayerLog.h"

#include "HostWindow.h"
#include "threading/MainThread.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>

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
      std::format(
          "player_set_display(OVERLAY, elm_win) {}, DST_ROI {} in {}, {}",
          playerErrorName(bound),
          playerErrorName(mode),
          millisSince(started),
          threadDescription()));
  display_.bound = bound == PLAYER_ERROR_NONE;
  display_.visible = true;
  setVisible(false);
  notifyDisplayChange();
}

void TizenPlayer::open(const std::string &uri, const SourceHeaders &headers) {
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
          playerLog(
              number,
              std::format(
                  "unprepare {} in {}, {}", playerErrorName(unprepared), millisSince(started), threadDescription()));
          return handle;
        },
        [uri, headers](TizenPlayer &self) { self.open(uri, headers); });
    return;
  }
  phase_ = Phase::Idle;
  seek_.inFlight = false;
  seek_.waitsForStart = false;
  seek_.pending.reset();
  pendingTracks_.clear();
  display_.subtitlesShown = false;
  duration_ = 0;
  video_ = {};
  display_.firstFrameOwner = nullptr;
  refreshPlaying();
  uri_ = uri;
  sourceSet_ = false;
  if (uri_.empty()) {
    setStatus(PlayerStatus::Idle);
    return;
  }
  const int set = player_set_uri(handle_, uri_.c_str());
  playerLog(number_, std::format("player_set_uri {}: {}", playerErrorName(set), uri_));
  if (set != PLAYER_ERROR_NONE) {
    fail("The player refused the source: " + playerErrorName(set));
    return;
  }
  sourceSet_ = true;
  applyHeaders(headers);
}

void TizenPlayer::prepare() {
  if (lending_.lent) {
    lending_.deferred.emplace_back([](TizenPlayer &self) { self.prepare(); });
    return;
  }
  if (handle_ == nullptr || phase_ != Phase::Idle || !sourceSet_) return;
  if (settings_.prebufferMs > 0) {
    playerLog(
        number_,
        std::format(
            "prebuffer {} ms {}",
            settings_.prebufferMs,
            playerErrorName(player_set_streaming_buffering_time(handle_, settings_.prebufferMs, -1))));
  }
  phase_ = Phase::Preparing;
  setStatus(PlayerStatus::Loading);
  auto result = std::make_shared<int>(PLAYER_ERROR_NONE);
  lend(
      [number = number_, target = target_.get(), result](player_h handle) {
        const auto started = std::chrono::steady_clock::now();
        *result = player_prepare_async(handle, preparedCallback, target);
        playerLog(
            number,
            std::format(
                "player_prepare_async {} in {}, {}",
                playerErrorName(*result),
                millisSince(started),
                threadDescription()));
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
  seek_.generation = generation;
  const auto clamped = std::max(0.0, duration_ > 0 ? std::min(seconds, duration_) : seconds);
  seek_.pending = std::make_pair(static_cast<int>(std::lround(clamped * 1000)), accurate);
  pump();
}

void TizenPlayer::pump() {
  if (handle_ == nullptr || phase_ != Phase::Ready || seek_.inFlight) return;
  auto current = state();
  if (seek_.pending && !(seek_.waitsForStart && current == PLAYER_STATE_READY) && issueSeek(current)) return;
  if (wantsPlaying_ && current != PLAYER_STATE_PLAYING) {
    const int started = player_start(handle_);
    playerLog(number_, std::format("player_start {}", playerErrorName(started)));
    if (!display_.visible) player_set_display_visible(handle_, false);
  } else if (!wantsPlaying_ && current == PLAYER_STATE_PLAYING) {
    playerLog(number_, std::format("player_pause {}", playerErrorName(player_pause(handle_))));
  }
  refreshPlaying();
  current = state();
  if (current != PLAYER_STATE_READY) {
    retryTracks();
    if (seek_.pending && seek_.waitsForStart) issueSeek(current);
  }
}

bool TizenPlayer::issueSeek(player_state_e current) {
  const auto [milliseconds, accurate] = *seek_.pending;
  const int sought = player_set_play_position(handle_, milliseconds, accurate, seekCompletedCallback, target_.get());
  playerLog(
      number_, std::format("seek to {} ms in {}: {}", milliseconds, playerStateName(current), playerErrorName(sought)));
  if (sought == PLAYER_ERROR_NONE) {
    seek_.inFlight = true;
    seek_.waitsForStart = false;
    seek_.targetMs = milliseconds;
    seek_.pending.reset();
    return true;
  }
  if (current == PLAYER_STATE_READY && !seek_.waitsForStart) {
    seek_.waitsForStart = true;
    playerLog(number_, "the seek waits for playback to start");
    return false;
  }
  seek_.pending.reset();
  seek_.waitsForStart = false;
  return false;
}

void TizenPlayer::selectTrack(TrackKind kind, int index) {
  if (kind == TrackKind::Text && index < 0) {
    display_.subtitlesShown = false;
    pendingTracks_.erase(kind);
    if (display_.presentation && display_.presentation->onSubtitle) display_.presentation->onSubtitle({}, 0);
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
      std::format(
          "select {} track {} in {}: {}",
          kind == TrackKind::Audio ? "audio" : "text",
          index,
          playerStateName(current),
          playerErrorName(selected)));
  if (selected == PLAYER_ERROR_NONE) {
    pendingTracks_.erase(kind);
    if (kind == TrackKind::Text) display_.subtitlesShown = true;
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

void TizenPlayer::fail(const std::string &message) {
  playerLog(number_, message, LogLevel::Error);
  wantsPlaying_ = false;
  setStatus(PlayerStatus::Error, message);
  refreshPlaying();
}

void TizenPlayer::setStatus(PlayerStatus status, const std::string &error) {
  if (status == status_ && status != PlayerStatus::Error) return;
  playerLog(number_, std::format("status {} -> {}", statusName(status_), statusName(status)));
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
