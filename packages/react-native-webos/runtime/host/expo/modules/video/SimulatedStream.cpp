#include "SimulatedStream.h"

#include <cstdlib>
#include <string_view>

namespace rnwebos::expo {

namespace {

constexpr std::string_view kMarker = "#sim:";
constexpr std::size_t kMaxTracks = 16;

double numberFrom(std::string_view text, double fallback) {
  const std::string digits(text);
  char *end = nullptr;
  const double value = std::strtod(digits.c_str(), &end);
  return !digits.empty() && end == digits.c_str() + digits.size() ? value : fallback;
}

std::vector<MediaTrack> languages(std::string_view list) {
  std::vector<MediaTrack> tracks;
  while (!list.empty() && tracks.size() < kMaxTracks) {
    const auto plus = list.find('+');
    const auto language = list.substr(0, plus);
    tracks.push_back({.index = static_cast<int>(tracks.size()), .language = language.empty() ? "und" : std::string(language)});
    list = plus == std::string_view::npos ? std::string_view() : list.substr(plus + 1);
  }
  return tracks;
}

void apply(SimulatedStream &stream, std::string_view name, std::string_view value) {
  auto &media = stream.media;
  if (name == "duration") media.duration = numberFrom(value, media.duration);
  if (name == "load") stream.loadDelay = numberFrom(value, stream.loadDelay);
  if (name == "error") stream.loadError = static_cast<int>(numberFrom(value, 0));
  if (name == "error-at") stream.errorAt = numberFrom(value, -1);
  if (name == "audio") media.audioTracks = languages(value);
  if (name == "text") media.textTracks = languages(value);
  if (name == "size") {
    const auto by = value.find('x');
    if (by == std::string_view::npos) return;
    media.video.width = static_cast<int>(numberFrom(value.substr(0, by), media.video.width));
    media.video.height = static_cast<int>(numberFrom(value.substr(by + 1), media.video.height));
  }
}

} // namespace

SimulatedStream simulatedStreamFor(const std::string &uri) {
  SimulatedStream stream{
      .media = {
          .duration = 60,
          .audioTracks = {{.index = 0, .language = "en"}, {.index = 1, .language = "fr"}},
          .textTracks = {{.index = 0, .language = "en"}},
          .audioTrack = 0,
          .textTrack = -1,
          .video = {.width = 1920, .height = 1080, .fps = 30, .bitrate = 5000000, .codec = "h264"},
      },
  };
  const auto marker = uri.find(kMarker);
  if (marker == std::string::npos) return stream;
  std::string_view rest = std::string_view(uri).substr(marker + kMarker.size());
  while (!rest.empty()) {
    const auto next = rest.find('&');
    const auto pair = rest.substr(0, next);
    if (const auto equals = pair.find('='); equals != std::string_view::npos) {
      apply(stream, pair.substr(0, equals), pair.substr(equals + 1));
    }
    rest = next == std::string_view::npos ? std::string_view() : rest.substr(next + 1);
  }
  if (stream.media.audioTracks.empty()) stream.media.audioTrack = -1;
  return stream;
}

} // namespace rnwebos::expo
