#include "VideoPlayerHarness.test.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::awaitEvents;
using testing::canonical;
using testing::createVideoPlayer;
using testing::eventCount;
using testing::ExpoTestRuntime;
using testing::lastEvent;

TEST_CASE("ExpoVideo answers that webOS has no picture in picture and no video cache") {
  ExpoTestRuntime expo;
  CHECK(expo.json("expo.modules.ExpoVideo.isPictureInPictureSupported()") == "false");
  CHECK(expo.json("expo.modules.ExpoVideo.getCurrentVideoCacheSize()") == "0");
  CHECK(expo.settle("expo.modules.ExpoVideo.setVideoCacheSizeAsync(1024)") == "undefined");
  CHECK(expo.settle("expo.modules.ExpoVideo.clearVideoCacheAsync()") == "undefined");
}

TEST_CASE("a VideoPlayer opening a source loads it, then is ready to play with what the stream holds") {
  ExpoTestRuntime expo;
  const auto native = createVideoPlayer(
      expo, "{ uri: 'https://example.com/a.mp4#sim:load=0.05&duration=42&size=1280x720', headers: { 'User-Agent': 'TV', X: 5 } }");
  REQUIRE(native != nullptr);
  CHECK(native->mediaPlayer() != nullptr);
  CHECK(expo.json("player.status") == "\"loading\"");
  REQUIRE(awaitEvents(expo, "statusChange"));
  CHECK(lastEvent(expo, "statusChange") == R"([{"oldStatus":"loading","status":"readyToPlay"}])");
  CHECK(expo.json("events.map(([n]) => n)") ==
        R"(["sourceLoad","availableAudioTracksChange","availableSubtitleTracksChange","audioTrackChange","videoTrackChange","statusChange"])");
  CHECK(canonical(expo, "[player.status, player.playing, player.duration, player.videoTrack.size, player.audioTrack.id,"
                        " player.availableAudioTracks.map(track => track.language), player.availableSubtitleTracks.length]") ==
        R"(["readyToPlay",false,42,{"height":720,"width":1280},"audio-0",["en","fr"],1])");
}

TEST_CASE("a source the pipeline cannot open ends in the error status, with the pipeline's reason") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "'https://example.com/missing.mp4#sim:load=0&error=202'");
  REQUIRE(awaitEvents(expo, "statusChange"));
  CHECK(lastEvent(expo, "statusChange") ==
        R"([{"error":{"message":"The TV cannot open the source: media not found (202): simulated"},)"
        R"("oldStatus":"loading","status":"error"}])");
  CHECK(expo.json("[player.status, player.playing]") == R"(["error",false])");
}

TEST_CASE("a VideoPlayer plays, pauses, seeks and reaches the end of its stream, and plays it again") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "'https://example.com/a.mp4#sim:load=0&duration=1'");
  REQUIRE(awaitEvents(expo, "statusChange"));
  expo.eval("player.timeUpdateEventInterval = 0.1; player.play();");
  REQUIRE(awaitEvents(expo, "playingChange"));
  CHECK(lastEvent(expo, "playingChange") == R"([{"isPlaying":true,"oldIsPlaying":false}])");
  REQUIRE(awaitEvents(expo, "timeUpdate", 2));
  CHECK(expo.eval("player.currentTime > 0 && player.currentTime <= 1 && player.bufferedPosition === 1").getBool());

  expo.eval("player.pause();");
  REQUIRE(awaitEvents(expo, "playingChange", 2));
  expo.eval("player.currentTime = 0.6;");
  CHECK(expo.json("player.currentTime") == "0.6");
  REQUIRE(awaitEvents(expo, "statusChange", 3));
  CHECK(expo.json("events.filter(([n]) => n === 'statusChange').map(([, e]) => e.status)") ==
        R"(["readyToPlay","loading","readyToPlay"])");
  CHECK(expo.json("[player.currentTime, player.playing]") == "[0.6,false]");

  expo.eval("player.play();");
  REQUIRE(awaitEvents(expo, "playToEnd"));
  REQUIRE(awaitEvents(expo, "playingChange", 4));
  CHECK(expo.json("[player.currentTime, player.playing, player.status]") == R"([1,false,"readyToPlay"])");

  expo.eval("player.replay();");
  REQUIRE(awaitEvents(expo, "playToEnd", 2));
  CHECK(eventCount(expo, "playingChange") >= 6);
}

TEST_CASE("a looping VideoPlayer starts over at the end and keeps playing") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "'https://example.com/a.mp4#sim:load=0&duration=0.4'");
  REQUIRE(awaitEvents(expo, "statusChange"));
  expo.eval("player.loop = true; player.play();");
  REQUIRE(awaitEvents(expo, "playToEnd", 2));
  CHECK(expo.json("player.playing") == "true");
  CHECK(eventCount(expo, "playingChange") == 1);
}

TEST_CASE("a VideoPlayer switches audio tracks and turns the pipeline's subtitles on and off") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "'https://example.com/a.mp4#sim:load=0&audio=en+fr&text=en+de'");
  REQUIRE(awaitEvents(expo, "statusChange"));
  expo.eval("player.audioTrack = player.availableAudioTracks[1]; player.subtitleTrack = { language: 'de' };");
  REQUIRE(awaitEvents(expo, "subtitleTrackChange"));
  CHECK(expo.json("[player.audioTrack.id, player.subtitleTrack.id]") == R"(["audio-1","text-1"])");
  CHECK(expo.json("(([, e]) => [e.audioTrack.language, e.oldAudioTrack.language])("
                  "events.filter(([n]) => n === 'audioTrackChange').at(-1))") == R"(["fr","en"])");
  expo.eval("player.subtitleTrack = null;");
  REQUIRE(awaitEvents(expo, "subtitleTrackChange", 2));
  CHECK(expo.json("player.subtitleTrack") == "null");
}

TEST_CASE("a VideoPlayer without a source starts idle") {
  ExpoTestRuntime expo;
  const auto native = createVideoPlayer(expo, "null");
  CHECK(expo.json("player.status") == "\"idle\"");
  CHECK(expo.json("[player.duration, player.bufferedPosition, player.currentTime]") == "[0,-1,0]");
  const auto plain = createVideoPlayer(expo, "'https://example.com/b.mp4'");
  CHECK(expo.json("player.status") == "\"loading\"");
  CHECK(plain->number() > native->number());
}

TEST_CASE("VideoPlayer keeps the playback settings JavaScript sets, and reports the changes") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "null");
  expo.eval("player.muted = true; player.muted = true;");
  expo.eval("player.volume = 0.25; player.volume = 5; player.volume = 1;");
  expo.eval("player.playbackRate = 2; player.playbackRate = 2;");
  expo.eval("player.loop = true; player.timeUpdateEventInterval = -1;");
  CHECK(expo.json("[player.muted, player.volume, player.playbackRate, player.loop, player.timeUpdateEventInterval]") ==
        "[true,1,2,true,0]");
  expo.eval("player.volume = -3; player.timeUpdateEventInterval = 0.5;");
  CHECK(expo.json("[player.volume, player.timeUpdateEventInterval]") == "[0,0.5]");
  REQUIRE(awaitEvents(expo, "volumeChange", 3));
  CHECK(eventCount(expo, "mutedChange") == 1);
  CHECK(lastEvent(expo, "mutedChange") == R"([{"muted":true,"oldMuted":false}])");
  CHECK(expo.json("events.filter(([n]) => n === 'volumeChange').map(([, e]) => [e.volume, e.oldVolume])") ==
        "[[0.25,1],[1,0.25],[0,1]]");
  CHECK(eventCount(expo, "playbackRateChange") == 1);
  CHECK(lastEvent(expo, "playbackRateChange") == R"([{"oldPlaybackRate":1,"playbackRate":2}])");
}

TEST_CASE("VideoPlayer keeps buffer options, seek tolerance and the resolution cap as JavaScript sets them") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "null");
  CHECK(expo.json("[player.bufferOptions, player.seekTolerance, player.maxResolution]") == "[{},{},null]");
  expo.eval(
      "player.bufferOptions = { minBufferForPlayback: 3 };"
      "player.seekTolerance = { toleranceBefore: 0, toleranceAfter: 0 };"
      "player.maxResolution = { width: 1280, height: 720 };");
  CHECK(canonical(expo, "[player.bufferOptions, player.seekTolerance, player.maxResolution]") ==
        R"([{"minBufferForPlayback":3},{"toleranceAfter":0,"toleranceBefore":0},{"height":720,"width":1280}])");
  expo.eval("player.currentTime = 4; player.bufferOptions = 7; player.seekTolerance = 'exact'; player.maxResolution = null;");
  CHECK(expo.json("[player.bufferOptions, player.seekTolerance, player.maxResolution, player.currentTime]") ==
        "[{},{},null,4]");
}

TEST_CASE("VideoPlayer moves its position with currentTime, seekBy and replay") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "null");
  expo.eval("player.currentTime = 12.5; player.seekBy(2.5);");
  CHECK(expo.json("player.currentTime") == "15");
  expo.eval("player.currentTime = -3;");
  CHECK(expo.json("player.currentTime") == "0");
  expo.eval("player.currentTime = 30; player.replay(); player.play(); player.pause();");
  CHECK(expo.json("player.currentTime") == "0");
}

TEST_CASE("VideoPlayer answers what webOS does not play, and keeps the options it has no player feature for") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "null");
  CHECK(expo.json(
            "[player.isLive, player.currentLiveTimestamp, player.currentOffsetFromLive, player.isExternalPlaybackActive,"
            " player.audioTrack, player.subtitleTrack, player.videoTrack, player.availableAudioTracks,"
            " player.availableSubtitleTracks, player.availableVideoTracks]") ==
        "[false,null,null,false,null,null,null,[],[],[]]");
  CHECK(expo.json(
            "[player.targetOffsetFromLive, player.preservesPitch, player.keepScreenOnWhilePlaying,"
            " player.showNowPlayingNotification, player.staysActiveInBackground, player.allowsExternalPlayback,"
            " player.audioMixingMode, player.scrubbingModeOptions]") == R"([0,true,true,false,false,false,"auto",{}])");
  expo.eval(
      "player.targetOffsetFromLive = 5; player.preservesPitch = false; player.keepScreenOnWhilePlaying = false;"
      "player.showNowPlayingNotification = true; player.staysActiveInBackground = true;"
      "player.allowsExternalPlayback = true; player.audioMixingMode = 'doNotMix';"
      "player.scrubbingModeOptions = { scrubbingModeEnabled: true };");
  CHECK(expo.json(
            "[player.targetOffsetFromLive, player.preservesPitch, player.keepScreenOnWhilePlaying,"
            " player.showNowPlayingNotification, player.staysActiveInBackground, player.allowsExternalPlayback,"
            " player.audioMixingMode, player.scrubbingModeOptions]") ==
        R"([5,false,false,true,true,true,"doNotMix",{"scrubbingModeEnabled":true}])");
}

TEST_CASE("replace and replaceAsync report the source change and open the new source") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "'https://example.com/a.mp4'");
  REQUIRE(awaitEvents(expo, "statusChange"));
  expo.eval("player.replace('https://example.com/b.mp4');");
  CHECK(expo.json("player.status") == "\"loading\"");
  REQUIRE(awaitEvents(expo, "sourceChange"));
  CHECK(lastEvent(expo, "sourceChange") ==
        R"([{"oldSource":"https://example.com/a.mp4","source":"https://example.com/b.mp4"}])");
  REQUIRE(awaitEvents(expo, "statusChange", 2));
  CHECK(expo.settle("player.replaceAsync({ uri: 'https://example.com/c.mp4' })") == "undefined");
  REQUIRE(awaitEvents(expo, "sourceChange", 2));
  CHECK(lastEvent(expo, "sourceChange") ==
        R"([{"oldSource":"https://example.com/b.mp4","source":{"uri":"https://example.com/c.mp4"}}])");
}

TEST_CASE("generateThumbnailsAsync rejects as unsupported, and a released player refuses its members") {
  ExpoTestRuntime expo;
  createVideoPlayer(expo, "null");
  CHECK_THROWS_WITH(
      expo.settle("player.generateThumbnailsAsync([1], {})"),
      "ERR_UNSUPPORTED: generateThumbnailsAsync is not supported on webOS");
  expo.eval("player.release();");
  CHECK(expo.json("player.__expo_shared_object_id__") == "0");
  CHECK(expo.json("(() => { try { return player.volume; } catch (error) { return error.message; } })()") ==
        R"("The native object was released or is of another class")");
}

} // namespace rnwebos::expo
