#include "VideoPlayerHarness.test.h"

#include <doctest/doctest.h>

namespace rntizen::expo {

using testing::awaitEvents;
using testing::canonical;
using testing::createVideoPlayer;
using testing::eventCount;
using testing::ExpoTestRuntime;
using testing::lastEvent;

namespace {

LoadedMedia twoLanguages() {
  return LoadedMedia{
      .duration = 42,
      .audioTracks = {{.index = 0, .language = "en"}, {.index = 1, .language = "und"}},
      .textTracks = {{.index = 0, .language = "fr"}},
      .audioTrack = 1,
      .textTrack = -1,
      .video = {.width = 1920, .height = 1080, .fps = 25, .bitrate = 5000000, .codec = "h264"},
  };
}

constexpr auto kSource = "'https://example.com/a.mp4'";

std::shared_ptr<VideoPlayer> settledPlayer(ExpoTestRuntime &expo) {
  auto native = createVideoPlayer(expo, kSource);
  REQUIRE(awaitEvents(expo, "statusChange"));
  return native;
}

} // namespace

TEST_CASE("a player's status change reaches JavaScript once, and every error does") {
  ExpoTestRuntime expo;
  const auto native = settledPlayer(expo);
  native->playerStatusChanged(PlayerStatus::ReadyToPlay, "");
  native->playerStatusChanged(PlayerStatus::ReadyToPlay, "");
  native->playerStatusChanged(PlayerStatus::Error, "first");
  native->playerStatusChanged(PlayerStatus::Error, "second");
  REQUIRE(awaitEvents(expo, "statusChange", 4));
  CHECK(canonical(expo, "events.filter(([n]) => n === 'statusChange').slice(1).map(([, e]) => e)") ==
        R"([{"oldStatus":"error","status":"readyToPlay"},)"
        R"({"error":{"message":"first"},"oldStatus":"readyToPlay","status":"error"},)"
        R"({"error":{"message":"second"},"oldStatus":"error","status":"error"}])");
  CHECK(expo.json("player.status") == "\"error\"");
}

TEST_CASE("playing, progress and the end of the media reach JavaScript and the player's position") {
  ExpoTestRuntime expo;
  const auto native = settledPlayer(expo);
  native->playerPlayingChanged(true);
  native->playerProgress(10, 20, 60, 0, true);
  REQUIRE(awaitEvents(expo, "timeUpdate"));
  CHECK(lastEvent(expo, "playingChange") == R"([{"isPlaying":true,"oldIsPlaying":false}])");
  CHECK(expo.json("[player.playing, player.duration, player.bufferedPosition]") == "[true,60,20]");
  CHECK(expo.eval("player.currentTime >= 10 && player.currentTime < 11").getBool());
  CHECK(native->isPlaying());
  CHECK(expo.json("(([, e]) => [e.currentLiveTimestamp, e.currentOffsetFromLive, e.bufferedPosition])("
                  "events.find(([n]) => n === 'timeUpdate'))") == "[null,null,20]");

  native->playerPlayingChanged(false);
  REQUIRE(awaitEvents(expo, "playingChange", 2));
  CHECK(lastEvent(expo, "playingChange") == R"([{"isPlaying":false,"oldIsPlaying":true}])");
  CHECK_FALSE(native->isPlaying());

  expo.eval("player.currentTime = 30;");
  native->playerProgress(5, 21, 0, 0, false);
  CHECK(expo.json("[player.currentTime, player.duration, player.bufferedPosition]") == "[30,60,21]");
  native->playerProgress(31, 40, 61, 1, false);
  CHECK(expo.json("[player.currentTime, player.duration]") == "[31,61]");
  expo.eval("player.currentTime = 100;");
  CHECK(expo.json("player.currentTime") == "61");
  CHECK(eventCount(expo, "timeUpdate") == 1);

  native->playerEnded();
  REQUIRE(awaitEvents(expo, "playToEnd"));
  CHECK(lastEvent(expo, "playToEnd") == "[]");
}

TEST_CASE("the media a player loads reaches JavaScript as its source, duration and tracks") {
  ExpoTestRuntime expo;
  const auto native = settledPlayer(expo);
  native->playerLoaded(twoLanguages());
  REQUIRE(awaitEvents(expo, "videoTrackChange"));
  CHECK(expo.json("events.slice(1).map(([n]) => n)") ==
        R"(["sourceLoad","availableAudioTracksChange","availableSubtitleTracksChange","audioTrackChange","videoTrackChange"])");
  CHECK(expo.json("(([, e]) => [e.videoSource, e.duration, e.availableVideoTracks.length, e.availableAudioTracks.length,"
                  " e.availableSubtitleTracks.length])(events.find(([n]) => n === 'sourceLoad'))") ==
        R"(["https://example.com/a.mp4",42,1,2,1])");
  CHECK(lastEvent(expo, "audioTrackChange").find(R"("id":"audio-1")") != std::string::npos);
  CHECK(canonical(expo, "[player.duration, player.audioTrack.label, player.subtitleTrack, player.availableAudioTracks.length,"
                        " player.availableSubtitleTracks[0].language, player.videoTrack.size, player.availableVideoTracks.length]") ==
        R"([42,"Track 2",null,2,"fr",{"height":1080,"width":1920},1])");

  native->playerVideoChanged({.width = 1280, .height = 720, .fps = 0, .bitrate = 0, .codec = ""});
  REQUIRE(awaitEvents(expo, "videoTrackChange", 2));
  CHECK(expo.json("(([, e]) => [e.videoTrack.size.width, e.videoTrack.mimeType, e.oldVideoTrack.size.width])("
                  "events.filter(([n]) => n === 'videoTrackChange').at(-1))") == R"([1280,null,1920])");

  native->playerTrackChanged(TrackKind::Audio, 0);
  native->playerTrackChanged(TrackKind::Text, 0);
  REQUIRE(awaitEvents(expo, "subtitleTrackChange"));
  CHECK(expo.json("(([, e]) => [e.audioTrack.id, e.oldAudioTrack.id])(events.filter(([n]) => n === 'audioTrackChange').at(-1))") ==
        R"(["audio-0","audio-1"])");
  CHECK(expo.json("(([, e]) => [e.subtitleTrack.id, e.oldSubtitleTrack])(events.find(([n]) => n === 'subtitleTrackChange'))") ==
        R"(["text-0",null])");
  CHECK(expo.json("[player.audioTrack.id, player.subtitleTrack.language]") == R"(["audio-0","fr"])");
}

TEST_CASE("JavaScript picks tracks by id or language, and turning subtitles off reaches it back") {
  ExpoTestRuntime expo;
  const auto native = settledPlayer(expo);
  native->playerLoaded(twoLanguages());
  native->playerTrackChanged(TrackKind::Text, 0);
  REQUIRE(awaitEvents(expo, "subtitleTrackChange"));
  expo.eval(
      "player.audioTrack = { id: 'audio-0' }; player.audioTrack = { language: 'de' }; player.audioTrack = null;"
      "player.subtitleTrack = { language: 'fr' }; player.subtitleTrack = { id: 'text-9' };");
  expo.eval("player.subtitleTrack = null;");
  REQUIRE(awaitEvents(expo, "subtitleTrackChange", 2));
  CHECK(expo.json("(([, e]) => [e.subtitleTrack, e.oldSubtitleTrack.id])(events.filter(([n]) => n === 'subtitleTrackChange').at(-1))") ==
        R"([null,"text-0"])");
  CHECK(expo.json("player.subtitleTrack") == "null");
}

TEST_CASE("the TV's media session drives the player and reads it back") {
  ExpoTestRuntime expo;
  const auto native = settledPlayer(expo);
  MediaSession &session = *native;
  CHECK_FALSE(session.staysActiveInBackground());
  expo.eval("player.staysActiveInBackground = 'yes';");
  CHECK_FALSE(session.staysActiveInBackground());
  expo.eval("player.staysActiveInBackground = true;");
  CHECK(session.staysActiveInBackground());

  session.seekTo(12);
  CHECK(expo.json("player.currentTime") == "12");
  session.seekBy(3);
  CHECK(expo.json("player.currentTime") == "15");
  session.restart();
  CHECK(expo.json("player.currentTime") == "0");
  session.play();
  session.pause();

  CHECK_FALSE(session.setSubtitlesEnabled(true));
  native->playerLoaded(twoLanguages());
  native->playerTrackChanged(TrackKind::Text, 0);
  REQUIRE(awaitEvents(expo, "subtitleTrackChange"));
  CHECK(session.setSubtitlesEnabled(true));
  CHECK(session.setSubtitlesEnabled(false));
  REQUIRE(awaitEvents(expo, "subtitleTrackChange", 2));
  CHECK(expo.json("player.subtitleTrack") == "null");
}

TEST_CASE("a player JavaScript does not hold, or whose app is gone, reports nothing") {
  ExpoTestRuntime expo;
  const auto unpaired = std::make_shared<VideoPlayer>(expo.context());
  CHECK(unpaired->tizenPlayer() == nullptr);
  unpaired->setMuted(true);
  unpaired->playerEnded();
  unpaired->setSubtitleTrack(nullptr);
  CHECK(unpaired->muted());

  auto orphan = std::make_shared<VideoPlayer>(std::weak_ptr<Context>());
  orphan->open(folly::dynamic::object("uri", "https://example.com/a.mp4"));
  CHECK(orphan->tizenPlayer() != nullptr);
  orphan->setVolume(0.5);
  orphan->playerEnded();
  CHECK(expo.pumpUntil([&orphan] { return orphan->status() == PlayerStatus::Error; }));
  CHECK(orphan->volume() == 0.5);
}

} // namespace rntizen::expo
