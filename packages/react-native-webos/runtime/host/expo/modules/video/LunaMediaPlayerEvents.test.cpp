#include "LunaMediaPlayerHarness.test.h"

#include "PipelineQuestions.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::FakeLunaBus;
using testing::kAccepted;
using testing::kLoadCompleted;
using testing::kLoaded;
using testing::kSourceInfo;
using testing::pumpFor;
using testing::pumpUntil;
using testing::RecordingListener;

TEST_CASE("sourceInfo then loadCompleted report the media once, with its duration, tracks and video") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.load();
  bus.emit(R"({"subscription":true})");
  bus.emit(R"({"sourceInfo":{"programInfo":[{"duration":0,"numAudioTracks":0,"numVideoTracks":0}]}})");
  bus.emit(kSourceInfo);
  CHECK(listener.lines.empty());
  bus.emit(kLoadCompleted);
  bus.emit(kLoadCompleted);
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1"});
  CHECK(listener.loaded.audioTracks[1].language == "fre");
  CHECK(listener.loaded.audioTrack == 0);
  CHECK(listener.loaded.video.codec == "h264");
}

TEST_CASE("a sourceInfo that comes after loadCompleted reports the media again") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.load();
  bus.emit(kLoadCompleted);
  CHECK(listener.lines == std::vector<std::string>{"loaded 0.0 0x0 0+0"});
  bus.emit(kSourceInfo);
  CHECK(listener.lines.back() == "loaded 120.0 1920x1080 2+1");
}

TEST_CASE("videoInfo sets the size before the media is loaded, and reports a change after") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.load();
  bus.emit(R"({"videoInfo":{"width":1280,"height":720,"frameRate":50,"bitRate":2000000,"mediaId":"_xyz"}})");
  bus.emit(kSourceInfo);
  bus.emit(kLoadCompleted);
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1280x720 2+1"});
  CHECK(listener.loaded.video == VideoStream{.width = 1280, .height = 720, .fps = 50, .bitrate = 2000000, .codec = "h264"});

  bus.emit(R"({"videoInfo":{"width":1280,"height":720,"frameRate":50,"bitRate":2000000,"mediaId":"_xyz"}})");
  bus.emit(R"({"videoInfo":{"width":3840,"height":2160,"frameRate":50,"bitRate":9000000,"mediaId":"_xyz"}})");
  CHECK(listener.lines.back() == "video 3840x2160");
  CHECK(listener.lines.size() == 2);
}

TEST_CASE("position, buffer and buffering events reach the listener in seconds") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.emit(R"({"currentTime":{"currentTime":4200,"mediaId":"_xyz"}})");
  bus.emit(R"({"bufferRange":{"beginTime":0,"endTime":23,"remainingTime":97,"percent":19,"mediaId":"_xyz"}})");
  CHECK(listener.position == doctest::Approx(4.2));
  CHECK(listener.buffered == doctest::Approx(23));
  bus.emit(R"({"bufferRange":{"beginTime":0,"endTime":118,"remainingTime":-1,"percent":100,"mediaId":"_xyz"}})");
  CHECK(listener.buffered == doctest::Approx(120));
  bus.emit(R"({"bufferingStart":{"mediaId":"_xyz"}})");
  bus.emit(R"({"bufferingEnd":{"mediaId":"_xyz"}})");
  bus.emit(R"({"endOfStream":{"mediaId":"_xyz"}})");
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1", "buffering", "buffered", "ended"});
}

TEST_CASE("a load the TV denies fails with the TV's own words") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  player.load({.uri = "https://example.com/a.mp4", .headers = {}});
  bus.answer("load", R"({"errorCode":-1,"returnValue":false,"errorText":"Denied method call \"load\" for category \"/\""})");
  CHECK(listener.lines ==
        std::vector<std::string>{R"(failed: The TV cannot open the source: error (-1): Denied method call "load" for category "/")"});
  CHECK(bus.methods() == "load");
}

TEST_CASE("a bus that takes no call fails the load once the caller has returned") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  bus.refuseCalls = true;
  player.load({.uri = "https://example.com/a.mp4", .headers = {}});
  CHECK(listener.lines.empty());
  CHECK(pumpUntil([&] { return !listener.lines.empty(); }));
  CHECK(listener.lines.front() == "failed: The TV's media service did not take the load");
}

TEST_CASE("an error event ends playback when the media cannot go on, and only then") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.emit(R"({"error":{"errorCode":210,"errorText":"unknown subtitle","mediaId":"_xyz"}})");
  bus.emit(R"({"error":{"errorCode":61441,"errorText":"buffer full","mediaId":"_xyz"}})");
  CHECK(listener.lines.size() == 1);
  bus.emit(R"({"error":{"errorCode":202,"errorText":"Media not found","mediaId":"_xyz"}})");
  CHECK(listener.lines.back() == "failed: The TV's player failed: media not found (202): Media not found");
}

TEST_CASE("an unload nobody asked for fails the player: the TV took the pipeline") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.emit(R"({"unloadCompleted":{"mediaId":"_xyz"}})");
  CHECK(listener.lines.back() == "failed: The TV took the video pipeline away");
  player.play();
  CHECK(bus.count("play") == 0);
  CHECK(pumpUntil([&] { return bus.openSubscriptions() == 0; }));
}

TEST_CASE("the next load waits for the unload of the media before it") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.completesUnloads = false;
  player.load({.uri = "https://example.com/b.mp4", .headers = {}});
  CHECK(bus.payloadOf("unload") == R"({"mediaId":"_xyz"})");
  CHECK(bus.count("load") == 1);
  bus.emit(kLoadCompleted);
  bus.emit(R"({"unloadCompleted":{"mediaId":"_xyz"}})");
  CHECK(pumpUntil([&] { return bus.count("load") == 2; }));
  CHECK(bus.openSubscriptions() == 0);
  CHECK(listener.lines.size() == 1);
  bus.answer("load", R"({"returnValue":true,"mediaId":"_next"})");
  CHECK(bus.payloadOf("subscribe") == R"({"mediaId":"_next","subscribe":true})");
  bus.completesUnloads = true;
}

TEST_CASE("a load goes ahead when the pipeline never says the media before it is unloaded") {
  testing::LunaFixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.completesUnloads = false;
  player.load({.uri = "https://example.com/b.mp4", .headers = {}});
  CHECK(bus.count("load") == 1);
  CHECK(pumpUntil([&] { return bus.count("load") == 2; }));
}

TEST_CASE("a media whose load is answered after it was replaced is unloaded, and a destroyed player unloads its own") {
  FakeLunaBus bus;
  {
    rnwebos::testing::MainLoopScope mainLoop;
    RecordingListener listener;
    LunaMediaPlayer player(bus, std::make_unique<testing::FixedPlane>(), listener, "com.example.app", 1);
    player.load({.uri = "https://example.com/a.mp4", .headers = {}});
    player.unload();
    bus.answer("load", R"({"returnValue":true,"mediaId":"_late"})");
    CHECK(bus.payloadOf("unload") == R"({"mediaId":"_late"})");
    CHECK(bus.count("subscribe") == 0);
    player.load({.uri = "https://example.com/b.mp4", .headers = {}});
    CHECK(bus.count("load") == 1);
    CHECK(pumpUntil([&] { return bus.count("load") == 2; }));
    bus.answer("load", kLoaded);
    CHECK(listener.lines.empty());
  }
  CHECK(bus.payloadOf("unload") == R"({"mediaId":"_xyz"})");
  bus.emit(kLoadCompleted);
}

} // namespace rnwebos::expo
