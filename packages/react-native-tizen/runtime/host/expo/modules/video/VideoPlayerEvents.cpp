#include "VideoPlayer.h"

#include "VideoTracks.h"

#include "../../RuntimeState.h"

#include <EventEmitter.h>
#include <jsi/JSIDynamic.h>

namespace rntizen::expo {

void VideoPlayer::emit(const std::string &event, folly::dynamic payload) const {
  auto context = context_.lock();
  if (!context) return;
  context->runOnJs([self = weak_from_this(), event, payload = std::move(payload)](facebook::jsi::Runtime &runtime) {
    auto object = self.lock();
    if (!object || object->sharedObjectId() == 0) return;
    auto target = RuntimeState::get(runtime).liveObject(runtime, object->sharedObjectId());
    if (!target) return;
    facebook::jsi::Value argument = facebook::jsi::valueFromDynamic(runtime, payload);
    ::expo::EventEmitter::emitEvent(runtime, *target, event, &argument, payload.isNull() ? 0 : 1);
  });
}

void VideoPlayer::playerStatusChanged(PlayerStatus status, const std::string &error) {
  PlayerStatus old;
  {
    std::lock_guard lock(mutex_);
    old = std::exchange(snapshot_.status, status);
  }
  if (old == status && status != PlayerStatus::Error) return;
  folly::dynamic payload = folly::dynamic::object("status", statusName(status))("oldStatus", statusName(old));
  if (status == PlayerStatus::Error) payload["error"] = folly::dynamic::object("message", error);
  emit("statusChange", std::move(payload));
}

void VideoPlayer::playerPlayingChanged(bool playing) {
  {
    std::lock_guard lock(mutex_);
    snapshot_.position = positionLocked();
    snapshot_.positionAt = std::chrono::steady_clock::now();
    snapshot_.playing = playing;
  }
  if (playing) markMediaSessionActive(this);
  emit("playingChange", folly::dynamic::object("isPlaying", playing)("oldIsPlaying", !playing));
}

void VideoPlayer::playerLoaded(const LoadedMedia &media) {
  folly::dynamic source;
  {
    std::lock_guard lock(mutex_);
    snapshot_.media = media;
    snapshot_.duration = media.duration;
    source = snapshot_.source;
  }
  const auto audio = tracksPayload(TrackKind::Audio, media.audioTracks, media.audioTrack);
  const auto text = tracksPayload(TrackKind::Text, media.textTracks, media.textTrack);
  folly::dynamic video = folly::dynamic::array();
  if (auto track = videoTrackPayload(media.video); !track.isNull()) video.push_back(std::move(track));
  emit("sourceLoad", folly::dynamic::object("videoSource", source)("duration", media.duration)(
                         "availableVideoTracks", video)("availableSubtitleTracks", text)("availableAudioTracks", audio));
  emit("availableAudioTracksChange", folly::dynamic::object("availableAudioTracks", audio));
  emit("availableSubtitleTracksChange", folly::dynamic::object("availableSubtitleTracks", text));
  emit("audioTrackChange",
       folly::dynamic::object("audioTrack", currentTrackPayload(TrackKind::Audio, media.audioTracks, media.audioTrack)));
  emit("videoTrackChange", folly::dynamic::object("videoTrack", videoTrackPayload(media.video)));
}

void VideoPlayer::playerProgress(double position, double buffered, double duration, unsigned seekGeneration, bool tick) {
  folly::dynamic payload;
  {
    std::lock_guard lock(mutex_);
    if (seekGeneration >= snapshot_.seekGeneration) {
      snapshot_.position = position;
      snapshot_.positionAt = std::chrono::steady_clock::now();
    }
    snapshot_.buffered = buffered;
    if (duration > 0) snapshot_.duration = duration;
    if (tick) {
      payload = folly::dynamic::object("currentTime", positionLocked())("currentLiveTimestamp", nullptr)(
          "currentOffsetFromLive", nullptr)("bufferedPosition", buffered);
    }
  }
  if (tick) emit("timeUpdate", std::move(payload));
}

void VideoPlayer::playerEnded() {
  emit("playToEnd", nullptr);
}

void VideoPlayer::playerVideoChanged(const VideoStream &video) {
  VideoStream old;
  {
    std::lock_guard lock(mutex_);
    old = std::exchange(snapshot_.media.video, video);
  }
  emit("videoTrackChange",
       folly::dynamic::object("videoTrack", videoTrackPayload(video))("oldVideoTrack", videoTrackPayload(old)));
}

void VideoPlayer::playerTrackChanged(TrackKind kind, int index) {
  using enum TrackKind;
  folly::dynamic payload;
  {
    std::lock_guard lock(mutex_);
    auto &media = snapshot_.media;
    auto &current = kind == Audio ? media.audioTrack : media.textTrack;
    const auto &tracks = kind == Audio ? media.audioTracks : media.textTracks;
    const auto old = std::exchange(current, index);
    const char *name = kind == Audio ? "audioTrack" : "subtitleTrack";
    const char *oldName = kind == Audio ? "oldAudioTrack" : "oldSubtitleTrack";
    payload = folly::dynamic::object(name, currentTrackPayload(kind, tracks, index))(
        oldName, currentTrackPayload(kind, tracks, old));
  }
  emit(kind == Audio ? "audioTrackChange" : "subtitleTrackChange", std::move(payload));
}

folly::dynamic VideoPlayer::audioTrack() const {
  std::lock_guard lock(mutex_);
  const auto &media = snapshot_.media;
  return currentTrackPayload(TrackKind::Audio, media.audioTracks, media.audioTrack);
}

void VideoPlayer::setAudioTrack(const folly::dynamic &track) {
  std::optional<int> index;
  {
    std::lock_guard lock(mutex_);
    index = trackIndexOf(TrackKind::Audio, track, snapshot_.media.audioTracks);
  }
  if (index.has_value()) command([index = *index](TizenPlayer &tizen) { tizen.selectTrack(TrackKind::Audio, index); });
}

folly::dynamic VideoPlayer::subtitleTrack() const {
  std::lock_guard lock(mutex_);
  const auto &media = snapshot_.media;
  return currentTrackPayload(TrackKind::Text, media.textTracks, media.textTrack);
}

void VideoPlayer::setSubtitleTrack(const folly::dynamic &track) {
  std::optional<int> index = -1;
  if (!track.isNull()) {
    std::lock_guard lock(mutex_);
    index = trackIndexOf(TrackKind::Text, track, snapshot_.media.textTracks);
  }
  if (index.has_value()) command([index = *index](TizenPlayer &tizen) { tizen.selectTrack(TrackKind::Text, index); });
}

folly::dynamic VideoPlayer::availableAudioTracks() const {
  std::lock_guard lock(mutex_);
  return tracksPayload(TrackKind::Audio, snapshot_.media.audioTracks, snapshot_.media.audioTrack);
}

folly::dynamic VideoPlayer::availableSubtitleTracks() const {
  std::lock_guard lock(mutex_);
  return tracksPayload(TrackKind::Text, snapshot_.media.textTracks, snapshot_.media.textTrack);
}

folly::dynamic VideoPlayer::videoTrack() const {
  std::lock_guard lock(mutex_);
  return videoTrackPayload(snapshot_.media.video);
}

folly::dynamic VideoPlayer::availableVideoTracks() const {
  auto track = videoTrack();
  folly::dynamic list = folly::dynamic::array();
  if (!track.isNull()) list.push_back(std::move(track));
  return list;
}

} // namespace rntizen::expo
