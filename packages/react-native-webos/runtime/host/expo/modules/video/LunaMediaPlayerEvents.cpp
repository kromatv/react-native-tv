#include "LunaMediaPlayer.h"

#include "PipelineQuestions.h"
#include "PlayerLog.h"

#include <folly/json.h>

#include <format>

namespace rnwebos::expo {

namespace {

constexpr std::size_t kMaxLoggedEvent = 600;

std::string excerpt(const folly::dynamic &message) {
  auto json = folly::toJson(message);
  if (json.size() > kMaxLoggedEvent) json.resize(kMaxLoggedEvent);
  return json;
}

} // namespace

void LunaMediaPlayer::noteFirst(const LunaMediaEvent &event, const folly::dynamic &message) {
  if (!seenEvents_.insert(event.name).second) return;
  playerLog(number_, std::format("first {}: {}", event.name, excerpt(message)), LogLevel::Debug);
  if (event.name == "subtitleData") {
    pipeline::openQuestion(number_, "the pipeline sends subtitle text (subtitleData): an overlay could draw it");
  }
}

void LunaMediaPlayer::onEvent(const folly::dynamic &message) {
  using enum LunaMediaEvent::Kind;
  const auto event = parseLunaMediaEvent(message);
  noteFirst(event, message);
  switch (event.kind) {
    case LoadCompleted:
      return onLoadCompleted();
    case SourceInfo:
      return onSourceInfo(event);
    case VideoInfo:
      return onVideoInfo(event.video);
    case CurrentTime:
      return listener_.backendPosition(event.seconds);
    case BufferRange:
      return listener_.backendBuffered(event.complete && media_.duration > 0 ? media_.duration : event.seconds);
    case BufferingStart:
      return listener_.backendBuffering(true);
    case BufferingEnd:
      return listener_.backendBuffering(false);
    case Playing:
      return setPlaying(true);
    case Paused:
      return setPlaying(false);
    case SeekDone:
      return finishSeek();
    case EndOfStream:
      playing_ = false;
      return listener_.backendEnded();
    case TrackSelected:
      return confirmTrack(event.trackKind, event.trackIndex);
    case Error:
      return onError(event);
    case UnloadCompleted:
      return onUnloaded();
    case Reply:
      if (!event.accepted) fail("The TV refused the subscription: " + describeMediaError(event.errorCode, event.errorText));
      return;
    case Other:
      return;
  }
}

void LunaMediaPlayer::onLoadCompleted() {
  if (std::exchange(loadCompleted_, true)) return;
  if (!sourceKnown_) pipeline::openQuestion(number_, "loadCompleted before any sourceInfo: ready without a duration");
  listener_.backendLoaded(media_);
}

void LunaMediaPlayer::onSourceInfo(const LunaMediaEvent &event) {
  if (event.placeholder) {
    playerLog(number_, "sourceInfo without a duration or a track: skipped", LogLevel::Debug);
    return;
  }
  const int audioTrack = sourceKnown_ ? media_.audioTrack : event.media.audioTrack;
  media_ = event.media;
  media_.audioTrack = audioTrack;
  media_.textTrack = textTrack_;
  sourceVideo_ = event.media.video;
  media_.video = currentVideo();
  sourceKnown_ = true;
  if (!loadCompleted_) return;
  pipeline::openQuestion(number_, "sourceInfo after loadCompleted: the media is reported again");
  listener_.backendLoaded(media_);
}

VideoStream LunaMediaPlayer::currentVideo() const {
  auto video = sourceVideo_;
  if (liveVideo_.width <= 0 || liveVideo_.height <= 0) return video;
  video.width = liveVideo_.width;
  video.height = liveVideo_.height;
  if (liveVideo_.fps > 0) video.fps = liveVideo_.fps;
  if (liveVideo_.bitrate > 0) video.bitrate = liveVideo_.bitrate;
  return video;
}

void LunaMediaPlayer::onVideoInfo(const VideoStream &video) {
  if (liveVideo_.width > 0 && (video.width != liveVideo_.width || video.height != liveVideo_.height)) {
    pipeline::openQuestion(number_, std::format("videoInfo again, now {}x{}: a rendition change", video.width, video.height));
  }
  liveVideo_ = video;
  const auto current = currentVideo();
  if (current == media_.video) return;
  media_.video = current;
  if (loadCompleted_) listener_.backendVideoChanged(current);
}

void LunaMediaPlayer::onError(const LunaMediaEvent &event) {
  const auto what = describeMediaError(event.errorCode, event.errorText);
  if (pipeline::endsPlayback(event.errorCode)) {
    fail("The TV's player failed: " + what);
    return;
  }
  playerLog(number_, "the pipeline reports " + what + ": playback goes on", LogLevel::Warning);
  if (event.errorCode == 700) finishSeek();
}

void LunaMediaPlayer::onUnloaded() {
  pipeline::openQuestion(number_, "unloadCompleted nobody asked for: the pipeline was taken away");
  mediaId_.clear();
  later([this] {
    if (mediaId_.empty()) events_.reset();
  });
  fail("The TV took the video pipeline away");
}

void LunaMediaPlayer::setPlaying(bool playing) {
  if (awaited_.playing == playing) {
    awaited_.playing.reset();
    awaited_.playWait.cancel();
  }
  if (std::exchange(playing_, playing) != playing) listener_.backendPlayingChanged(playing);
}

void LunaMediaPlayer::finishSeek() {
  awaited_.seekWait.cancel();
  if (std::exchange(awaited_.seek, false)) listener_.backendSeekDone();
}

void LunaMediaPlayer::confirmTrack(TrackKind kind, int index) {
  if (awaited_.track == Track(kind, index)) {
    awaited_.track.reset();
    awaited_.trackWait.cancel();
  }
  auto &current = kind == TrackKind::Audio ? media_.audioTrack : textTrack_;
  if (std::exchange(current, index) != index) listener_.backendTrackChanged(kind, index);
}

} // namespace rnwebos::expo
