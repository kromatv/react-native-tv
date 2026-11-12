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

TEST_CASE("a load goes out with the plane's window id, then foreground and subscribe follow its media id") {
  testing::LunaFixture fixture;
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

TEST_CASE("play and pause are calls on the media, confirmed by the playing and paused events") {
  testing::LunaFixture fixture;
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
  testing::LunaFixture fixture;
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

TEST_CASE("a seek sends milliseconds and is done at seekDone, or after a while without one") {
  testing::LunaFixture fixture;
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
  testing::LunaFixture fixture;
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
  testing::LunaFixture fixture;
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
  testing::LunaFixture fixture;
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
  testing::LunaFixture fixture;
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

} // namespace rnwebos::expo
