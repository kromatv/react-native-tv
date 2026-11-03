#include "VideoTracks.h"

#include <string>

namespace rntizen::expo {

namespace {

std::string trackId(TrackKind kind, int index) {
  return std::string(kind == TrackKind::Audio ? "audio-" : "text-") + std::to_string(index);
}

std::string stringField(const folly::dynamic &object, const char *name) {
  const auto *field = object.isObject() ? object.get_ptr(name) : nullptr;
  return field != nullptr && field->isString() ? field->getString() : std::string();
}

} // namespace

folly::dynamic trackPayload(TrackKind kind, const MediaTrack &track, int current) {
  const auto label = track.language == "und" ? "Track " + std::to_string(track.index + 1) : track.language;
  return folly::dynamic::object("id", trackId(kind, track.index))("language", track.language)("label", label)(
      "name", label)("isDefault", track.index == 0)("autoSelect", track.index == current);
}

folly::dynamic tracksPayload(TrackKind kind, const std::vector<MediaTrack> &tracks, int current) {
  folly::dynamic list = folly::dynamic::array();
  for (const auto &track : tracks) list.push_back(trackPayload(kind, track, current));
  return list;
}

folly::dynamic currentTrackPayload(TrackKind kind, const std::vector<MediaTrack> &tracks, int current) {
  for (const auto &track : tracks) {
    if (track.index == current) return trackPayload(kind, track, current);
  }
  return nullptr;
}

folly::dynamic videoTrackPayload(const VideoStream &video) {
  if (video.width <= 0 || video.height <= 0) return nullptr;
  const folly::dynamic bitrate = video.bitrate > 0 ? folly::dynamic(video.bitrate) : folly::dynamic(nullptr);
  const folly::dynamic fps = video.fps > 0 ? folly::dynamic(video.fps) : folly::dynamic(nullptr);
  const folly::dynamic codec = video.codec.empty() ? folly::dynamic(nullptr) : folly::dynamic(video.codec);
  return folly::dynamic::object("id", "video-0")("url", nullptr)(
      "size", folly::dynamic::object("width", video.width)("height", video.height))("mimeType", codec)(
      "isSupported", true)("bitrate", bitrate)("averageBitrate", nullptr)("peakBitrate", nullptr)("frameRate", fps);
}

std::optional<int> trackIndexOf(TrackKind kind, const folly::dynamic &track, const std::vector<MediaTrack> &tracks) {
  if (!track.isObject()) return std::nullopt;
  const auto id = stringField(track, "id");
  const auto language = stringField(track, "language");
  for (const auto &candidate : tracks) {
    if (!id.empty() && id == trackId(kind, candidate.index)) return candidate.index;
  }
  for (const auto &candidate : tracks) {
    if (id.empty() && !language.empty() && language == candidate.language) return candidate.index;
  }
  return std::nullopt;
}

} // namespace rntizen::expo
