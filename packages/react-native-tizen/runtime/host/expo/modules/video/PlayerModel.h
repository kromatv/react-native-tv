#pragma once

#include <string>
#include <vector>

namespace rntizen::expo {

/// expo-video's `VideoPlayerStatus`.
enum class PlayerStatus { Idle, Loading, ReadyToPlay, Error };

const char *statusName(PlayerStatus status);

/// One audio or text track of the media, as capi-media-player lists it: an
/// index and an ISO 639-1 language code, `und` when the file names none.
struct MediaTrack {
  int index{0};
  std::string language;
};

/// The video stream playing now; zero where the player does not say.
struct VideoStream {
  int width{0};
  int height{0};
  int fps{0};
  int bitrate{0};
  std::string codec;

  bool operator==(const VideoStream &) const = default;
};

/// What the player learned once the source was prepared.
struct LoadedMedia {
  double duration{0};
  std::vector<MediaTrack> audioTracks;
  std::vector<MediaTrack> textTracks;
  int audioTrack{-1};
  int textTrack{-1};
  VideoStream video;
};

enum class TrackKind { Audio, Text };

/// expo-video's `contentFit`.
enum class ContentFit { Contain, Cover, Fill };

ContentFit contentFitFrom(const std::string &name);

} // namespace rntizen::expo
