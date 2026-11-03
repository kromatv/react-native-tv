#include "TizenPlayer.h"

#include "PlayerCallbackTarget.h"
#include "PlayerLog.h"

#include "threading/MainThread.h"

#include <cstdlib>

namespace rntizen::expo {

TizenPlayer::TizenPlayer(std::weak_ptr<PlayerObserver> observer, int number)
    : observer_(std::move(observer)), number_(number) {}

TizenPlayer::~TizenPlayer() {
  release();
}

void TizenPlayer::setCallbacks() {
  player_set_completed_cb(handle_, completedCallback, target_.get());
  player_set_buffering_cb(handle_, bufferingCallback, target_.get());
  player_set_error_cb(handle_, errorCallback, target_.get());
  player_set_interrupted_cb(handle_, interruptedCallback, target_.get());
  player_set_video_stream_changed_cb(handle_, videoStreamChangedCallback, target_.get());
  player_set_subtitle_updated_cb(handle_, subtitleCallback, target_.get());
}

void TizenPlayer::unsetCallbacks(player_h handle) {
  player_unset_completed_cb(handle);
  player_unset_buffering_cb(handle);
  player_unset_error_cb(handle);
  player_unset_interrupted_cb(handle);
  player_unset_video_stream_changed_cb(handle);
  player_unset_subtitle_updated_cb(handle);
}

void TizenPlayer::preparedCallback(void *data) {
  static_cast<CallbackTarget *>(data)->post("player_prepared_cb", [](TizenPlayer &player) { player.onPrepared(); });
}

void TizenPlayer::completedCallback(void *data) {
  static_cast<CallbackTarget *>(data)->post("player_completed_cb", [](TizenPlayer &player) { player.onCompleted(); });
}

void TizenPlayer::seekCompletedCallback(void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_seek_completed_cb", [](TizenPlayer &player) { player.onSeekCompleted(); });
}

void TizenPlayer::bufferingCallback(int percent, void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_buffering_cb", [percent](TizenPlayer &player) { player.onBuffering(percent); });
}

void TizenPlayer::errorCallback(int code, void *data) {
  static_cast<CallbackTarget *>(data)->post("player_error_cb", [code](TizenPlayer &player) { player.onError(code); });
}

void TizenPlayer::interruptedCallback(player_interrupted_code_e code, void *data) {
  auto *target = static_cast<CallbackTarget *>(data);
  playerLog(target->number, "interrupted with code " + std::to_string(code), LogLevel::Warning);
  target->post(
      "player_interrupted_cb", [code](TizenPlayer &player) { player.onInterrupted(code); });
}

void TizenPlayer::videoStreamChangedCallback(int width, int height, int fps, int bitrate, void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_video_stream_changed_cb",
      [=](TizenPlayer &player) { player.onVideoStreamChanged(width, height, fps, bitrate); });
}

void TizenPlayer::subtitleCallback(unsigned long duration, char *text, void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_subtitle_updated_cb", [duration, line = std::string(text != nullptr ? text : "")](TizenPlayer &player) {
        player.onSubtitle(line, duration);
      });
}

void TizenPlayer::onSubtitle(const std::string &text, unsigned long milliseconds) {
  playerLog(
      number_,
      "subtitle for " + std::to_string(milliseconds) + " ms" + (subtitlesShown_ ? ", shown: " : ", not selected: ") + text,
      LogLevel::Debug);
  if (!subtitlesShown_ || !presentation_ || !presentation_->onSubtitle) return;
  const auto callback = presentation_->onSubtitle;
  callback(text, milliseconds);
}

void TizenPlayer::onPrepared() {
  if (phase_ != Phase::Preparing) return;
  phase_ = Phase::Ready;
  auto media = readMedia();
  duration_ = media.duration;
  video_ = media.video;
  playerLog(
      number_,
      "prepared: " + std::to_string(video_.width) + "x" + std::to_string(video_.height) + " " + video_.codec + ", " +
          decimal(media.duration) + " s, " + std::to_string(media.audioTracks.size()) + " audio and " +
          std::to_string(media.textTracks.size()) + " text tracks");
  if (auto observer = observer_.lock()) observer->playerLoaded(media);
  setStatus(PlayerStatus::ReadyToPlay);
  if (rate_ != 1) setPlaybackRate(rate_);
  applyDisplay();
  startProgress();
  retryTracks();
  pump();
}

void TizenPlayer::onCompleted() {
  playerLog(number_, std::string("completed") + (looping_ ? ", looping" : ""));
  if (!looping_) {
    wantsPlaying_ = false;
    if (state() == PLAYER_STATE_PLAYING) player_pause(handle_);
    refreshPlaying();
  }
  if (auto observer = observer_.lock()) observer->playerEnded();
}

void TizenPlayer::onSeekCompleted() {
  seeking_ = false;
  int position = 0;
  player_get_play_position(handle_, &position);
  playerLog(
      number_, "seek completed at " + std::to_string(position) + " ms (target " + std::to_string(seekTargetMs_) + ")");
  reportProgress(false);
  pump();
}

void TizenPlayer::onBuffering(int percent) {
  if (phase_ != Phase::Ready) return;
  if (percent < 100 && status_ == PlayerStatus::ReadyToPlay) {
    playerLog(number_, "buffering " + std::to_string(percent) + " %");
    setStatus(PlayerStatus::Loading);
  } else if (percent >= 100 && status_ == PlayerStatus::Loading) {
    setStatus(PlayerStatus::ReadyToPlay);
  }
}

void TizenPlayer::onError(int code) {
  fail("The player failed: " + playerErrorName(code));
}

void TizenPlayer::onInterrupted(int code) {
  const auto current = state();
  playerLog(number_, "interrupted (" + std::to_string(code) + "), now " + playerStateName(current), LogLevel::Warning);
  refreshPlaying();
  if (current == PLAYER_STATE_IDLE) {
    phase_ = Phase::Idle;
    fail("Another player took the video decoder");
  }
}

void TizenPlayer::onVideoStreamChanged(int width, int height, int fps, int bitrate) {
  VideoStream video = video_;
  video.width = width;
  video.height = height;
  if (fps > 0) video.fps = fps;
  if (bitrate > 0) video.bitrate = bitrate;
  if (video == video_) return;
  video_ = video;
  playerLog(number_, "video stream " + std::to_string(width) + "x" + std::to_string(height) + " at " +
                         std::to_string(fps) + " fps, " + std::to_string(bitrate) + " b/s");
  if (auto observer = observer_.lock()) observer->playerVideoChanged(video_);
  applyDisplay();
}

void TizenPlayer::setProgressInterval(double seconds) {
  progressInterval_ = std::max(0.0, seconds);
  if (progress_ != nullptr) {
    stopProgress();
    startProgress();
  }
}

void TizenPlayer::startProgress() {
  if (progress_ != nullptr || phase_ != Phase::Ready) return;
  constexpr double kPollSeconds = 0.5;
  progress_ = ecore_timer_add(progressInterval_ > 0 ? progressInterval_ : kPollSeconds, progressTick, this);
}

void TizenPlayer::stopProgress() {
  if (progress_ == nullptr) return;
  ecore_timer_del(progress_);
  progress_ = nullptr;
}

Eina_Bool TizenPlayer::progressTick(void *data) {
  static_cast<TizenPlayer *>(data)->reportProgress(true);
  return ECORE_CALLBACK_RENEW;
}

void TizenPlayer::reportProgress(bool tick) {
  if (handle_ == nullptr || phase_ != Phase::Ready) return;
  int position = 0;
  if (seeking_ || pendingSeek_) {
    position = pendingSeek_ ? pendingSeek_->first : seekTargetMs_;
  } else {
    player_get_play_position(handle_, &position);
  }
  int duration = 0;
  if (player_get_duration(handle_, &duration) == PLAYER_ERROR_NONE && duration > 0) duration_ = duration / 1000.0;
  int start = 0;
  int end = 0;
  double buffered = -1;
  if (player_get_streaming_download_progress(handle_, &start, &end) == PLAYER_ERROR_NONE && duration_ > 0) {
    buffered = duration_ * end / 100.0;
  }
  if (auto observer = observer_.lock()) {
    observer->playerProgress(position / 1000.0, buffered, duration_, seekGeneration_, tick && progressInterval_ > 0);
  }
}

LoadedMedia TizenPlayer::readMedia() {
  LoadedMedia media;
  int duration = 0;
  if (player_get_duration(handle_, &duration) == PLAYER_ERROR_NONE) media.duration = duration / 1000.0;
  for (const auto kind : {TrackKind::Audio, TrackKind::Text}) {
    const auto type = kind == TrackKind::Audio ? PLAYER_STREAM_TYPE_AUDIO : PLAYER_STREAM_TYPE_TEXT;
    auto &tracks = kind == TrackKind::Audio ? media.audioTracks : media.textTracks;
    int count = 0;
    if (player_get_track_count(handle_, type, &count) != PLAYER_ERROR_NONE) count = 0;
    for (int index = 0; index < count; ++index) {
      char *code = nullptr;
      player_get_track_language_code(handle_, type, index, &code);
      tracks.push_back({.index = index, .language = code != nullptr && *code != '\0' ? code : "und"});
      std::free(code);
    }
    int current = -1;
    if (kind == TrackKind::Audio && count > 0 && player_get_current_track(handle_, type, &current) == PLAYER_ERROR_NONE) {
      media.audioTrack = current;
    }
  }
  media.video = readVideo();
  return media;
}

VideoStream TizenPlayer::readVideo() {
  VideoStream video;
  player_get_video_size(handle_, &video.width, &video.height);
  player_get_video_stream_info(handle_, &video.fps, &video.bitrate);
  char *audioCodec = nullptr;
  char *videoCodec = nullptr;
  if (player_get_codec_info(handle_, &audioCodec, &videoCodec) == PLAYER_ERROR_NONE && videoCodec != nullptr) {
    video.codec = videoCodec;
  }
  std::free(audioCodec);
  std::free(videoCodec);
  return video;
}

} // namespace rntizen::expo
