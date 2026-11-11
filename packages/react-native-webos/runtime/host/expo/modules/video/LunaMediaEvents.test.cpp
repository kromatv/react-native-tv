#include "LunaMediaEvents.h"

#include <doctest/doctest.h>
#include <folly/json.h>

namespace rnwebos::expo {

namespace {

using Kind = LunaMediaEvent::Kind;

LunaMediaEvent parse(const char *json) {
  return parseLunaMediaEvent(folly::parseJson(json));
}

constexpr auto kSourceInfo = R"({"sourceInfo":{"container":"mp4","numPrograms":1,"seekable":true,"trickable":true,
  "programInfo":[{"duration":596458,"numAudioTracks":2,
    "audioTrackInfo":[{"language":"eng","codec":"aac","profile":"lc","level":"","bitRate":128000,"sampleRate":48000,"channels":2,"audioType":0},
                      {"language":"","codec":"ac3","bitRate":384000,"sampleRate":48000,"channels":6}],
    "numVideoTracks":1,
    "videoTrackInfo":[{"angleNumber":0,"codec":"h264","profile":"high","level":4.1,"width":1920,"height":1080,"aspectRatio":"16:9","frameRate":23.976,"bitRate":3000000,"progressive":true}],
    "numSubtitleTracks":1,"subtitleType":"text_plain","subtitleTrackInfo":[{"language":"fre","pid":0,"ctag":0}]}],
  "mediaId":"_xyz"}})";

} // namespace

TEST_CASE("the events with no body are named by their key") {
  CHECK(parse(R"({"loadCompleted":{"mediaId":"_xyz"}})").kind == Kind::LoadCompleted);
  CHECK(parse(R"({"playing":{"mediaId":"_xyz"}})").kind == Kind::Playing);
  CHECK(parse(R"({"paused":{"mediaId":"_xyz"}})").kind == Kind::Paused);
  CHECK(parse(R"({"seekDone":{"mediaId":"_xyz"}})").kind == Kind::SeekDone);
  CHECK(parse(R"({"endOfStream":{"mediaId":"_xyz"}})").kind == Kind::EndOfStream);
  CHECK(parse(R"({"bufferingStart":{"mediaId":"_xyz"}})").kind == Kind::BufferingStart);
  CHECK(parse(R"({"bufferingEnd":{"mediaId":"_xyz"}})").kind == Kind::BufferingEnd);
  CHECK(parse(R"({"unloadCompleted":{"mediaId":"_xyz"}})").kind == Kind::UnloadCompleted);
  CHECK(parse(R"({"endOfStream":{"mediaId":"_xyz"}})").name == "endOfStream");
}

TEST_CASE("currentTime is the position in milliseconds") {
  const auto event = parse(R"({"currentTime":{"currentTime":12345,"mediaId":"_xyz"}})");
  CHECK(event.kind == Kind::CurrentTime);
  CHECK(event.seconds == doctest::Approx(12.345));
  CHECK(parse(R"({"currentTime":2000})").seconds == doctest::Approx(2));
}

TEST_CASE("bufferRange gives the end of the buffer in seconds, and says when all of it is loaded") {
  const auto partial =
      parse(R"({"bufferRange":{"beginTime":0,"endTime":17,"remainingTime":13,"percent":28,"mediaId":"_xyz"}})");
  CHECK(partial.kind == Kind::BufferRange);
  CHECK(partial.seconds == doctest::Approx(17));
  CHECK_FALSE(partial.complete);
  CHECK(parse(R"({"bufferRange":{"beginTime":0,"endTime":60,"remainingTime":-1,"percent":100}})").complete);
}

TEST_CASE("sourceInfo gives the duration, the tracks and the first video track of the first program") {
  const auto event = parse(kSourceInfo);
  CHECK(event.kind == Kind::SourceInfo);
  CHECK_FALSE(event.placeholder);
  const auto &media = event.media;
  CHECK(media.duration == doctest::Approx(596.458));
  REQUIRE(media.audioTracks.size() == 2);
  CHECK(media.audioTracks[0].language == "eng");
  CHECK(media.audioTracks[1].index == 1);
  CHECK(media.audioTracks[1].language == "und");
  CHECK(media.audioTrack == 0);
  REQUIRE(media.textTracks.size() == 1);
  CHECK(media.textTracks[0].language == "fre");
  CHECK(media.textTrack == -1);
  CHECK(media.video == VideoStream{.width = 1920, .height = 1080, .fps = 24, .bitrate = 3000000, .codec = "h264"});
}

TEST_CASE("sourceInfo without a program list is read as one program, and counts stand in for missing lists") {
  const auto event = parse(R"({"sourceInfo":{"duration":30000,"numAudioTracks":2,"numSubtitleTracks":1,"seekable":true}})");
  CHECK(event.media.duration == doctest::Approx(30));
  CHECK(event.media.audioTracks.size() == 2);
  CHECK(event.media.audioTracks[1].language == "und");
  CHECK(event.media.textTracks.size() == 1);
  CHECK(event.media.video.width == 0);
}

TEST_CASE("the sourceInfo the pipeline sends first, with nothing in it, is a placeholder") {
  const auto event = parse(
      R"({"sourceInfo":{"container":"","numPrograms":1,"programInfo":[{"duration":0,"numAudioTracks":0,"numVideoTracks":0}]}})");
  CHECK(event.kind == Kind::SourceInfo);
  CHECK(event.placeholder);
}

TEST_CASE("videoInfo gives the size, the frame rate and the bit rate playing now") {
  const auto event = parse(
      R"({"videoInfo":{"width":3840,"height":2160,"aspectRatio":"16:9","frameRate":59.94,"bitRate":15000000,"hdrType":"HDR10","SEI":[],"VUI":[],"mediaId":"_xyz"}})");
  CHECK(event.kind == Kind::VideoInfo);
  CHECK(event.video == VideoStream{.width = 3840, .height = 2160, .fps = 60, .bitrate = 15000000, .codec = ""});
}

TEST_CASE("trackSelected names the kind and the index") {
  const auto audio = parse(R"({"trackSelected":{"type":"audio","index":1,"mediaId":"_xyz"}})");
  CHECK(audio.kind == Kind::TrackSelected);
  CHECK(audio.trackKind == TrackKind::Audio);
  CHECK(audio.trackIndex == 1);
  const auto text = parse(R"({"trackSelected":{"type":"text","index":0,"mediaId":"_xyz"}})");
  CHECK(text.trackKind == TrackKind::Text);
  CHECK(text.trackIndex == 0);
}

TEST_CASE("an error carries its code and text, and a refused call reads the same way") {
  const auto error = parse(R"({"error":{"errorCode":202,"errorText":"Media not found","mediaId":"_xyz"}})");
  CHECK(error.kind == Kind::Error);
  CHECK(error.errorCode == 202);
  CHECK(error.errorText == "Media not found");

  const auto refused =
      parse(R"({"errorCode":-1,"returnValue":false,"errorText":"Denied method call \"load\" for category \"/\""})");
  CHECK(refused.kind == Kind::Reply);
  CHECK_FALSE(refused.accepted);
  CHECK(refused.errorCode == -1);
  CHECK(refused.errorText == R"(Denied method call "load" for category "/")");

  CHECK(parse(R"({"returnValue":true,"mediaId":"_xyz"})").accepted);
  CHECK(parse(R"({"subscription":true})").kind == Kind::Reply);
}

TEST_CASE("an event the player does not use keeps its name, and anything else is ignored") {
  const auto subtitle = parse(R"({"subtitleData":{"subtitleData":"Hello","presentationTime":1200}})");
  CHECK(subtitle.kind == Kind::Other);
  CHECK(subtitle.name == "subtitleData");
  CHECK(parseLunaMediaEvent(folly::dynamic::array(1, 2)).kind == Kind::Other);
  CHECK(parseLunaMediaEvent(nullptr).name.empty());
}

TEST_CASE("describeMediaError says what the TV's error code means") {
  CHECK(describeMediaError(202, "No such file") == "media not found (202): No such file");
  CHECK(describeMediaError(104, "") == "failed to load (104)");
  CHECK(describeMediaError(601, "") == "resource allocation failed (601)");
  CHECK(describeMediaError(40403, "HTTP 403") == "streaming protocol error (40403): HTTP 403");
  CHECK(describeMediaError(61442, "") == "buffer low (61442)");
  CHECK(describeMediaError(-1, "Denied") == "error (-1): Denied");
}

} // namespace rnwebos::expo
