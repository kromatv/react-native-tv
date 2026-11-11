#include "LunaMediaEvents.h"

#include "PipelineQuestions.h"

#include <array>
#include <cmath>
#include <format>
#include <string_view>
#include <utility>

namespace rnwebos::expo {

namespace {

using Kind = LunaMediaEvent::Kind;

constexpr std::array<std::pair<std::string_view, Kind>, 14> kEvents{{
    {"loadCompleted", Kind::LoadCompleted},
    {"sourceInfo", Kind::SourceInfo},
    {"videoInfo", Kind::VideoInfo},
    {"currentTime", Kind::CurrentTime},
    {"bufferRange", Kind::BufferRange},
    {"bufferingStart", Kind::BufferingStart},
    {"bufferingEnd", Kind::BufferingEnd},
    {"playing", Kind::Playing},
    {"paused", Kind::Paused},
    {"seekDone", Kind::SeekDone},
    {"endOfStream", Kind::EndOfStream},
    {"trackSelected", Kind::TrackSelected},
    {"error", Kind::Error},
    {"unloadCompleted", Kind::UnloadCompleted},
}};

constexpr std::array<std::pair<int, std::string_view>, 24> kErrors{{
    {100, "playing error"},
    {101, "command not supported"},
    {104, "failed to load"},
    {200, "audio codec not supported"},
    {201, "video codec not supported"},
    {202, "media not found"},
    {203, "AV type not found"},
    {204, "failed to demultiplex"},
    {210, "unknown subtitle"},
    {300, "network error"},
    {301, "unsecured Wi-Fi"},
    {400, "the HLS server answered 302"},
    {401, "the HLS server answered 404"},
    {402, "the HLS server answered 503"},
    {403, "the HLS server answered 504"},
    {500, "DRM error"},
    {506, "DRM error"},
    {600, "resource manager error"},
    {601, "resource allocation failed"},
    {700, "seek failure"},
    {701, "set play rate failure"},
    {1000, "the pipeline died"},
    {61441, "buffer full"},
    {61442, "buffer low"},
}};

const folly::dynamic *field(const folly::dynamic &object, std::string_view name) {
  return object.isObject() ? object.get_ptr(folly::StringPiece(name.data(), name.size())) : nullptr;
}

double number(const folly::dynamic &object, std::string_view name, double fallback = 0) {
  const auto *value = field(object, name);
  return value != nullptr && value->isNumber() ? value->asDouble() : fallback;
}

int integer(const folly::dynamic &object, std::string_view name, int fallback = 0) {
  return static_cast<int>(std::lround(number(object, name, fallback)));
}

std::string text(const folly::dynamic &object, std::string_view name) {
  const auto *value = field(object, name);
  return value != nullptr && value->isString() ? value->getString() : std::string();
}

std::vector<MediaTrack> tracks(const folly::dynamic &program, std::string_view list, std::string_view count) {
  std::vector<MediaTrack> result;
  if (const auto *entries = field(program, list); entries != nullptr && entries->isArray()) {
    for (const auto &entry : *entries) {
      const auto language = text(entry, "language");
      result.push_back({.index = static_cast<int>(result.size()), .language = language.empty() ? "und" : language});
    }
  }
  for (int index = static_cast<int>(result.size()); index < integer(program, count); ++index) {
    result.push_back({.index = index, .language = "und"});
  }
  return result;
}

VideoStream videoStream(const folly::dynamic &info) {
  return {
      .width = integer(info, "width"),
      .height = integer(info, "height"),
      .fps = integer(info, "frameRate"),
      .bitrate = integer(info, "bitRate"),
      .codec = text(info, "codec"),
  };
}

// OSE nests the tracks in `programInfo[0]`; a message without it is read as one program.
void readSourceInfo(const folly::dynamic &info, LunaMediaEvent &event) {
  const auto *programs = field(info, "programInfo");
  const bool nested = programs != nullptr && programs->isArray() && !programs->empty();
  const folly::dynamic &program = nested ? (*programs)[0] : info;
  auto &media = event.media;
  media.duration = number(program, "duration") / 1000;
  media.audioTracks = tracks(program, "audioTrackInfo", "numAudioTracks");
  media.textTracks = tracks(program, "subtitleTrackInfo", "numSubtitleTracks");
  media.audioTrack = media.audioTracks.empty() ? -1 : 0;
  const auto *video = field(program, "videoTrackInfo");
  const bool hasVideo = video != nullptr && video->isArray() && !video->empty();
  if (hasVideo) media.video = videoStream((*video)[0]);
  event.placeholder = media.duration <= 0 && media.audioTracks.empty() && !hasVideo;
}

void readBufferRange(const folly::dynamic &range, LunaMediaEvent &event) {
  event.seconds = pipeline::bufferedSeconds(number(range, "endTime"));
  event.complete = number(range, "remainingTime") == -1;
}

void readError(const folly::dynamic &error, LunaMediaEvent &event) {
  event.errorCode = integer(error, "errorCode");
  event.errorText = text(error, "errorText");
}

} // namespace

LunaMediaEvent parseLunaMediaEvent(const folly::dynamic &message) {
  LunaMediaEvent event;
  if (!message.isObject()) return event;
  for (const auto &[name, kind] : kEvents) {
    const auto *body = field(message, name);
    if (body == nullptr) continue;
    event.kind = kind;
    event.name = name;
    if (kind == Kind::SourceInfo) readSourceInfo(*body, event);
    if (kind == Kind::VideoInfo) event.video = videoStream(*body);
    if (kind == Kind::CurrentTime) event.seconds = (body->isNumber() ? body->asDouble() : number(*body, "currentTime")) / 1000;
    if (kind == Kind::BufferRange) readBufferRange(*body, event);
    if (kind == Kind::Error) readError(*body, event);
    if (kind == Kind::TrackSelected) {
      event.trackKind = text(*body, "type") == "audio" ? TrackKind::Audio : TrackKind::Text;
      event.trackIndex = integer(*body, "index", -1);
    }
    return event;
  }
  if (const auto *accepted = field(message, "returnValue"); accepted != nullptr && accepted->isBool()) {
    event.kind = Kind::Reply;
    event.name = "returnValue";
    event.accepted = accepted->getBool();
    readError(message, event);
    return event;
  }
  if (field(message, "subscription") != nullptr) {
    event.kind = Kind::Reply;
    event.name = "subscription";
    return event;
  }
  for (const auto &[name, value] : message.items()) {
    if (name.isString()) event.name = name.getString();
    break;
  }
  return event;
}

std::string describeMediaError(int code, const std::string &text) {
  std::string_view meaning = code >= 40000 ? "streaming protocol error" : "error";
  for (const auto &[known, name] : kErrors) {
    if (known == code) meaning = name;
  }
  return text.empty() ? std::format("{} ({})", meaning, code) : std::format("{} ({}): {}", meaning, code, text);
}

} // namespace rnwebos::expo
