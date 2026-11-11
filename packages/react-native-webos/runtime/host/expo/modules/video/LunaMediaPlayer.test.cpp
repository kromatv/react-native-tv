#include "LunaMediaPlayer.h"

#include "FakeLunaBus.test.h"
#include "PipelineQuestions.h"
#include "VideoTestSupport.test.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

using testing::FakeLunaBus;
using testing::pumpFor;
using testing::pumpUntil;
using testing::RecordingListener;

namespace {

class FixedPlane final : public VideoPlane {
 public:
  std::string windowId() override {
    return "_Window_Id_39";
  }
  void show(Scene &, const PixelRect &, const PixelRect &) override {
    // The tests here are about the bus, not the plane.
  }
  void hide() override {
    // Nothing is shown.
  }
};

constexpr auto kLoaded = R"({"returnValue":true,"mediaId":"_xyz"})";
constexpr auto kSourceInfo = R"({"sourceInfo":{"container":"mp4","numPrograms":1,"seekable":true,
  "programInfo":[{"duration":120000,"numAudioTracks":2,"audioTrackInfo":[{"language":"eng","codec":"aac"},{"language":"fre","codec":"aac"}],
    "numVideoTracks":1,"videoTrackInfo":[{"codec":"h264","width":1920,"height":1080,"frameRate":25,"bitRate":3000000}],
    "numSubtitleTracks":1,"subtitleType":"text_plain","subtitleTrackInfo":[{"language":"eng"}]}],"mediaId":"_xyz"}})";
constexpr auto kLoadCompleted = R"({"loadCompleted":{"mediaId":"_xyz"}})";
constexpr auto kAccepted = R"({"returnValue":true,"mediaId":"_xyz"})";

struct Fixture {
  rnwebos::testing::MainLoopScope mainLoop;
  FakeLunaBus bus;
  RecordingListener listener;
  LunaMediaPlayer player{bus, std::make_unique<FixedPlane>(), listener, "com.example.app", 1};

  void load(const char *uri = "https://example.com/a.mp4") {
    player.load({.uri = uri, .headers = {}});
    bus.answer("load", kLoaded);
  }

  void loadUntilReady() {
    load();
    bus.emit(kSourceInfo);
    bus.emit(kLoadCompleted);
  }
};

} // namespace

TEST_CASE("a load goes out with the plane's window id, then foreground and subscribe follow its media id") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  player.load({.uri = "https://example.com/master.m3u8", .headers = {{"Cookie", "a=b"}}});
  CHECK(bus.methods() == "load");
  CHECK(bus.payloadOf("load") ==
        R"({"payload":{"mediaTransportType":"HLS","option":{"appId":"com.example.app",)"
        R"("bufferControl":{"userBufferCtrl":false},"transmission":{"httpHeader":{"cookies":"a=b"}},)"
        R"("useSeekableRanges":true,"windowId":"_Window_Id_39"}},"type":"media","uri":"https://example.com/master.m3u8"})");

  bus.answer("load", kLoaded);
  CHECK(bus.methods() == "load notifyForeground subscribe");
  CHECK(bus.payloadOf("notifyForeground") == R"({"connectionId":"_xyz"})");
  CHECK(bus.payloadOf("subscribe") == R"({"mediaId":"_xyz","subscribe":true})");
  CHECK(listener.lines.empty());
}

TEST_CASE("sourceInfo then loadCompleted report the media once, with its duration, tracks and video") {
  Fixture fixture;
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
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.load();
  bus.emit(kLoadCompleted);
  CHECK(listener.lines == std::vector<std::string>{"loaded 0.0 0x0 0+0"});
  bus.emit(kSourceInfo);
  CHECK(listener.lines.back() == "loaded 120.0 1920x1080 2+1");
}

TEST_CASE("videoInfo sets the size before the media is loaded, and reports a change after") {
  Fixture fixture;
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

TEST_CASE("play and pause are calls on the media, confirmed by the playing and paused events") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.play();
  CHECK(bus.payloadOf("play") == R"({"mediaId":"_xyz"})");
  bus.answer("play", kAccepted);
  bus.emit(R"({"playing":{"mediaId":"_xyz"}})");
  bus.emit(R"({"playing":{"mediaId":"_xyz"}})");
  player.pause();
  CHECK(bus.payloadOf("pause") == R"({"mediaId":"_xyz"})");
  bus.answer("pause", kAccepted);
  bus.emit(R"({"paused":{"mediaId":"_xyz"}})");
  pumpFor(pipeline::kPlayStateWait + std::chrono::milliseconds(200));
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1", "playing", "paused"});
}

TEST_CASE("a pause the pipeline accepts but never confirms counts as paused after a while") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.play();
  bus.answer("play", kAccepted);
  bus.emit(R"({"playing":{"mediaId":"_xyz"}})");
  player.pause();
  bus.answer("pause", kAccepted);
  CHECK_FALSE(listener.has("paused"));
  CHECK(pumpUntil([&] { return listener.has("paused"); }));
}

TEST_CASE("position, buffer and buffering events reach the listener in seconds") {
  Fixture fixture;
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

TEST_CASE("a seek sends milliseconds and is done at seekDone, or after a while without one") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.seek(30.5);
  CHECK(bus.payloadOf("seek") == R"({"mediaId":"_xyz","position":30500})");
  bus.answer("seek", kAccepted);
  bus.emit(R"({"seekDone":{"mediaId":"_xyz"}})");
  bus.emit(R"({"seekDone":{"mediaId":"_xyz"}})");
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1", "seekDone"});

  player.seek(5);
  bus.answer("seek", kAccepted);
  CHECK(pumpUntil([&] { return listener.lines.size() == 3; }));
  CHECK(listener.lines.back() == "seekDone");
}

TEST_CASE("a seek the pipeline refuses, or fails with error 700, is over at once and playback goes on") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.seek(500);
  bus.answer("seek", R"({"returnValue":false,"errorCode":700,"errorText":"seek failure"})");
  CHECK(listener.lines.back() == "seekDone");
  player.seek(600);
  bus.answer("seek", kAccepted);
  bus.emit(R"({"error":{"errorCode":700,"errorText":"seek failure","mediaId":"_xyz"}})");
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1", "seekDone", "seekDone"});
}

TEST_CASE("an audio track is selected by index and confirmed by trackSelected, or by the answer after a while") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.selectTrack(TrackKind::Audio, 1);
  CHECK(bus.payloadOf("selectTrack") == R"({"index":1,"mediaId":"_xyz","type":"audio"})");
  bus.answer("selectTrack", kAccepted);
  bus.emit(R"({"trackSelected":{"type":"audio","index":1,"mediaId":"_xyz"}})");
  bus.emit(R"({"trackSelected":{"type":"audio","index":1,"mediaId":"_xyz"}})");
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1", "audio track 1"});

  player.selectTrack(TrackKind::Audio, 0);
  bus.answer("selectTrack", kAccepted);
  CHECK(pumpUntil([&] { return listener.has("audio track 0"); }));
}

TEST_CASE("a text track is selected and the pipeline's own subtitles are turned on, and off again") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.selectTrack(TrackKind::Text, 0);
  CHECK(bus.payloadOf("selectTrack") == R"({"index":0,"mediaId":"_xyz","type":"text"})");
  CHECK(bus.payloadOf("setSubtitleEnable") == R"({"enable":true,"mediaId":"_xyz"})");
  bus.emit(R"({"trackSelected":{"type":"text","index":0,"mediaId":"_xyz"}})");
  CHECK(listener.lines.back() == "text track 0");

  player.selectTrack(TrackKind::Text, -1);
  CHECK(bus.payloadOf("setSubtitleEnable") == R"({"enable":false,"mediaId":"_xyz"})");
  CHECK(bus.count("selectTrack") == 1);
  player.selectTrack(TrackKind::Text, 0);
  bus.emit(R"({"trackSelected":{"type":"text","index":0,"mediaId":"_xyz"}})");
  CHECK(listener.lines == std::vector<std::string>{"loaded 120.0 1920x1080 2+1", "text track 0", "text track 0"});
}

TEST_CASE("rate, volume and the app's visibility are calls on the media") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  player.setRate(1.5);
  player.setVolume(0.5);
  player.setForeground(false);
  CHECK(bus.payloadOf("setPlayRate") == R"({"audioOutput":true,"mediaId":"_xyz","playRate":1.5})");
  CHECK(bus.payloadOf("setVolume") == R"({"ease":{"duration":0,"type":"Linear"},"mediaId":"_xyz","volume":50})");
  CHECK(bus.payloadOf("notifyBackground") == R"({"connectionId":"_xyz"})");
  bus.answer("setVolume", R"({"returnValue":false,"errorCode":101,"errorText":"command not supported"})");
  CHECK(listener.lines.size() == 1);
}

TEST_CASE("a load the TV denies fails with the TV's own words") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  player.load({.uri = "https://example.com/a.mp4", .headers = {}});
  bus.answer("load", R"({"errorCode":-1,"returnValue":false,"errorText":"Denied method call \"load\" for category \"/\""})");
  CHECK(listener.lines ==
        std::vector<std::string>{R"(failed: The TV cannot open the source: error (-1): Denied method call "load" for category "/")"});
  CHECK(bus.methods() == "load");
}

TEST_CASE("a bus that takes no call fails the load once the caller has returned") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  bus.refuseCalls = true;
  player.load({.uri = "https://example.com/a.mp4", .headers = {}});
  CHECK(listener.lines.empty());
  CHECK(pumpUntil([&] { return !listener.lines.empty(); }));
  CHECK(listener.lines.front() == "failed: The TV's media service did not take the load");
}

TEST_CASE("an error event ends playback when the media cannot go on, and only then") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.emit(R"({"error":{"errorCode":210,"errorText":"unknown subtitle","mediaId":"_xyz"}})");
  bus.emit(R"({"error":{"errorCode":61441,"errorText":"buffer full","mediaId":"_xyz"}})");
  CHECK(listener.lines.size() == 1);
  bus.emit(R"({"error":{"errorCode":202,"errorText":"Media not found","mediaId":"_xyz"}})");
  CHECK(listener.lines.back() == "failed: The TV's player failed: media not found (202): Media not found");
}

TEST_CASE("an unload nobody asked for fails the player: the TV took the pipeline") {
  Fixture fixture;
  auto &[mainLoop, bus, listener, player] = fixture;
  fixture.loadUntilReady();
  bus.emit(R"({"unloadCompleted":{"mediaId":"_xyz"}})");
  CHECK(listener.lines.back() == "failed: The TV took the video pipeline away");
  player.play();
  CHECK(bus.count("play") == 0);
  CHECK(pumpUntil([&] { return bus.openSubscriptions() == 0; }));
}

TEST_CASE("the next load waits for the unload of the media before it") {
  Fixture fixture;
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
  Fixture fixture;
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
    LunaMediaPlayer player(bus, std::make_unique<FixedPlane>(), listener, "com.example.app", 1);
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
