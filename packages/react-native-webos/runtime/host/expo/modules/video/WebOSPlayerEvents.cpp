#include "WebOSPlayer.h"

#include "PlayerCallbackTarget.h"
#include "PlayerLog.h"

#include "threading/MainThread.h"

#include <cstdlib>
#include <format>

namespace rnwebos::expo {

WebOSPlayer::WebOSPlayer(std::weak_ptr<PlayerObserver> observer, int number)
    : observer_(std::move(observer)), number_(number) {}

WebOSPlayer::~WebOSPlayer() {
  release();
}

void WebOSPlayer::setCallbacks() {
  player_set_completed_cb(handle_, completedCallback, target_.get());
  player_set_buffering_cb(handle_, bufferingCallback, target_.get());
  player_set_error_cb(handle_, errorCallback, target_.get());
  player_set_interrupted_cb(handle_, interruptedCallback, target_.get());
  player_set_video_stream_changed_cb(handle_, videoStreamChangedCallback, target_.get());
  player_set_subtitle_updated_cb(handle_, subtitleCallback, target_.get());
}

void WebOSPlayer::unsetCallbacks(player_h handle) {
  player_unset_completed_cb(handle);
  player_unset_buffering_cb(handle);
  player_unset_error_cb(handle);
  player_unset_interrupted_cb(handle);
  player_unset_video_stream_changed_cb(handle);
  player_unset_subtitle_updated_cb(handle);
}

void WebOSPlayer::preparedCallback(void *data) {
  static_cast<CallbackTarget *>(data)->post("player_prepared_cb", [](WebOSPlayer &player) { player.onPrepared(); });
}

void WebOSPlayer::completedCallback(void *data) {
  static_cast<CallbackTarget *>(data)->post("player_completed_cb", [](WebOSPlayer &player) { player.onCompleted(); });
}

void WebOSPlayer::seekCompletedCallback(void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_seek_completed_cb", [](WebOSPlayer &player) { player.onSeekCompleted(); });
}

void WebOSPlayer::bufferingCallback(int percent, void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_buffering_cb", [percent](WebOSPlayer &player) { player.onBuffering(percent); });
}

void WebOSPlayer::errorCallback(int code, void *data) {
  static_cast<CallbackTarget *>(data)->post("player_error_cb", [code](WebOSPlayer &player) { player.onError(code); });
}

void WebOSPlayer::interruptedCallback(player_interrupted_code_e code, void *data) {
  auto *target = static_cast<CallbackTarget *>(data);
  playerLog(target->number, std::format("interrupted with code {}", static_cast<int>(code)), LogLevel::Warning);
  target->post(
      "player_interrupted_cb", [code](WebOSPlayer &player) { player.onInterrupted(code); });
}

void WebOSPlayer::videoStreamChangedCallback(int width, int height, int fps, int bitrate, void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_video_stream_changed_cb",
      [=](WebOSPlayer &player) { player.onVideoStreamChanged(width, height, fps, bitrate); });
}

void WebOSPlayer::subtitleCallback(unsigned long duration, char *text, void *data) {
  static_cast<CallbackTarget *>(data)->post(
      "player_subtitle_updated_cb", [duration, line = std::string(text != nullptr ? text : "")](WebOSPlayer &player) {
        player.onSubtitle(line, duration);
      });
}

void WebOSPlayer::onSubtitle(const std::string &text, unsigned long milliseconds) {
  playerLog(
      number_,
      std::format("subtitle for {} ms, {}: {}", milliseconds, display_.subtitlesShown ? "shown" : "not selected", text),
      LogLevel::Debug);
  if (!display_.subtitlesShown || !display_.presentation || !display_.presentation->onSubtitle) return;
  const auto callback = display_.presentation->onSubtitle;
  callback(text, milliseconds);
}

void WebOSPlayer::onPrepared() {
  if (phase_ != Phase::Preparing) return;
  phase_ = Phase::Ready;
  auto media = readMedia();
  duration_ = media.duration;
  video_ = media.video;
  playerLog(
      number_,
      std::format(
          "prepared: {}x{} {}, {} s, {} audio and {} text tracks",
          video_.width,
          video_.height,
          video_.codec,
          decimal(media.duration),
          media.audioTracks.size(),
          media.textTracks.size()));
  if (auto observer = observer_.lock()) observer->playerLoaded(media);
  setStatus(PlayerStatus::ReadyToPlay);
  if (settings_.rate != 1) setPlaybackRate(settings_.rate);
  applyDisplay();
  startProgress();
  retryTracks();
  pump();
}

void WebOSPlayer::onCompleted() {
  playerLog(number_, settings_.looping ? "completed, looping" : "completed");
  if (!settings_.looping) {
    wantsPlaying_ = false;
    if (state() == PLAYER_STATE_PLAYING) player_pause(handle_);
    refreshPlaying();
  }
  if (auto observer = observer_.lock()) observer->playerEnded();
}

void WebOSPlayer::onSeekCompleted() {
  seek_.inFlight = false;
  int position = 0;
  player_get_play_position(handle_, &position);
  playerLog(number_, std::format("seek completed at {} ms (target {})", position, seek_.targetMs));
  reportProgress(false);
  pump();
}

void WebOSPlayer::onBuffering(int percent) {
  using enum PlayerStatus;
  if (phase_ != Phase::Ready) return;
  if (percent < 100 && status_ == ReadyToPlay) {
    playerLog(number_, std::format("buffering {} %", percent));
    setStatus(Loading);
  } else if (percent >= 100 && status_ == Loading) {
    setStatus(ReadyToPlay);
  }
}

void WebOSPlayer::onError(int code) {
  fail("The player failed: " + playerErrorName(code));
}

void WebOSPlayer::onInterrupted(int code) {
  const auto current = state();
  playerLog(number_, std::format("interrupted ({}), now {}", code, playerStateName(current)), LogLevel::Warning);
  refreshPlaying();
  if (current == PLAYER_STATE_IDLE) {
    phase_ = Phase::Idle;
    fail("Another player took the video decoder");
  }
}

void WebOSPlayer::onVideoStreamChanged(int width, int height, int fps, int bitrate) {
  VideoStream video = video_;
  video.width = width;
  video.height = height;
  if (fps > 0) video.fps = fps;
  if (bitrate > 0) video.bitrate = bitrate;
  if (video == video_) return;
  video_ = video;
  playerLog(number_, std::format("video stream {}x{} at {} fps, {} b/s", width, height, fps, bitrate));
  if (auto observer = observer_.lock()) observer->playerVideoChanged(video_);
  applyDisplay();
}

void WebOSPlayer::setProgressInterval(double seconds) {
  settings_.progressInterval = std::max(0.0, seconds);
  if (progress_ != nullptr) {
    stopProgress();
    startProgress();
  }
}

void WebOSPlayer::startProgress() {
  if (progress_ != nullptr || phase_ != Phase::Ready) return;
  constexpr double kPollSeconds = 0.5;
  const double interval = settings_.progressInterval > 0 ? settings_.progressInterval : kPollSeconds;
  progress_ = ecore_timer_add(interval, progressTick, this);
}

void WebOSPlayer::stopProgress() {
  if (progress_ == nullptr) return;
  ecore_timer_del(progress_);
  progress_ = nullptr;
}

Eina_Bool WebOSPlayer::progressTick(void *data) {
  static_cast<WebOSPlayer *>(data)->reportProgress(true);
  return ECORE_CALLBACK_RENEW;
}

void WebOSPlayer::reportProgress(bool tick) {
  if (handle_ == nullptr || phase_ != Phase::Ready) return;
  int position = 0;
  if (seek_.inFlight || seek_.pending) {
    position = seek_.pending ? seek_.pending->first : seek_.targetMs;
  } else {
    player_get_play_position(handle_, &position);
  }
  if (int duration = 0; player_get_duration(handle_, &duration) == PLAYER_ERROR_NONE && duration > 0) {
    duration_ = duration / 1000.0;
  }
  int start = 0;
  int end = 0;
  double buffered = -1;
  if (player_get_streaming_download_progress(handle_, &start, &end) == PLAYER_ERROR_NONE && duration_ > 0) {
    buffered = duration_ * end / 100.0;
  }
  if (auto observer = observer_.lock()) {
    const bool timed = tick && settings_.progressInterval > 0;
    observer->playerProgress(position / 1000.0, buffered, duration_, seek_.generation, timed);
  }
}

LoadedMedia WebOSPlayer::readMedia() {
  using enum TrackKind;
  LoadedMedia media;
  if (int duration = 0; player_get_duration(handle_, &duration) == PLAYER_ERROR_NONE) {
    media.duration = duration / 1000.0;
  }
  for (const auto kind : {Audio, Text}) {
    const auto type = kind == Audio ? PLAYER_STREAM_TYPE_AUDIO : PLAYER_STREAM_TYPE_TEXT;
    auto &tracks = kind == Audio ? media.audioTracks : media.textTracks;
    int count = 0;
    if (player_get_track_count(handle_, type, &count) != PLAYER_ERROR_NONE) count = 0;
    for (int index = 0; index < count; ++index) {
      char *code = nullptr;
      player_get_track_language_code(handle_, type, index, &code);
      tracks.push_back({.index = index, .language = code != nullptr && *code != '\0' ? code : "und"});
      std::free(code);
    }
    int current = -1;
    if (kind == Audio && count > 0 && player_get_current_track(handle_, type, &current) == PLAYER_ERROR_NONE) {
      media.audioTrack = current;
    }
  }
  media.video = readVideo();
  return media;
}

VideoStream WebOSPlayer::readVideo() {
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

} // namespace rnwebos::expo
