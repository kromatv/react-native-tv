#include "VideoPlayerHarness.test.h"

#include <doctest/doctest.h>

namespace rntizen::expo {

using testing::awaitEvents;
using testing::canonical;
using testing::createVideoPlayer;
using testing::eventCount;
using testing::ExpoTestRuntime;
using testing::kNoPlayer;
using testing::lastEvent;

TEST_CASE("ExpoVideo answers that Tizen has no picture in picture and no video cache") {
  ExpoTestRuntime expo;
  CHECK(expo.json("expo.modules.ExpoVideo.isPictureInPictureSupported()") == "false");
  CHECK(expo.json("expo.modules.ExpoVideo.getCurrentVideoCacheSize()") == "0");
  CHECK(expo.settle("expo.modules.ExpoVideo.setVideoCacheSizeAsync(1024)") == "undefined");
  CHECK(expo.settle("expo.modules.ExpoVideo.clearVideoCacheAsync()") == "undefined");
}

TEST_CASE("a VideoPlayer opening a source loads, then reports the error a TV without a player gives") {
  ExpoTestRuntime expo;
  const auto native = createVideoPlayer(expo, "{ uri: 'https://example.com/a.mp4', headers: { 'User-Agent': 'TV', X: 5 } }");
  REQUIRE(native != nullptr);
  CHECK(native->tizenPlayer() != nullptr);
  CHECK(expo.json("player.status") == "\"loading\"");
  REQUIRE(awaitEvents(expo, "statusChange"));
  CHECK(lastEvent(expo, "statusChange") ==
        std::string(R"([{"error":{"message":")") + kNoPlayer + R"("},"oldStatus":"loading","status":"error"}])");
  CHECK(expo.json("player.status") == "\"error\"");
  CHECK(expo.json("player.playing") == "false");
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

TEST_CASE("VideoPlayer answers what Tizen does not play, and keeps the options it has no player feature for") {
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
      "ERR_UNSUPPORTED: generateThumbnailsAsync is not supported on Tizen");
  expo.eval("player.release();");
  CHECK(expo.json("player.__expo_shared_object_id__") == "0");
  CHECK(expo.json("(() => { try { return player.volume; } catch (error) { return error.message; } })()") ==
        R"("The native object was released or is of another class")");
}

} // namespace rntizen::expo
