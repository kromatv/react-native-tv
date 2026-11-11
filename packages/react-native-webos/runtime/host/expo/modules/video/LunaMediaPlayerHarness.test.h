#pragma once

#include "LunaMediaPlayer.h"

#include "FakeLunaBus.test.h"
#include "VideoTestSupport.test.h"

#include <memory>
#include <string>

namespace rnwebos::expo::testing {

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

inline constexpr auto kLoaded = R"({"returnValue":true,"mediaId":"_xyz"})";
inline constexpr auto kSourceInfo = R"({"sourceInfo":{"container":"mp4","numPrograms":1,"seekable":true,
  "programInfo":[{"duration":120000,"numAudioTracks":2,"audioTrackInfo":[{"language":"eng","codec":"aac"},{"language":"fre","codec":"aac"}],
    "numVideoTracks":1,"videoTrackInfo":[{"codec":"h264","width":1920,"height":1080,"frameRate":25,"bitRate":3000000}],
    "numSubtitleTracks":1,"subtitleType":"text_plain","subtitleTrackInfo":[{"language":"eng"}]}],"mediaId":"_xyz"}})";
inline constexpr auto kLoadCompleted = R"({"loadCompleted":{"mediaId":"_xyz"}})";
inline constexpr auto kAccepted = R"({"returnValue":true,"mediaId":"_xyz"})";

struct LunaFixture {
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

} // namespace rnwebos::expo::testing
