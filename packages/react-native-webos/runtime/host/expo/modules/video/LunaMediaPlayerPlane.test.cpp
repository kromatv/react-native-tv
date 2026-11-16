#include "LunaMediaPlayerHarness.test.h"

#include <doctest/doctest.h>

#include <format>
#include <memory>
#include <string>
#include <vector>

namespace rnwebos::expo {

namespace {

using testing::FakeLunaBus;
using testing::kLoadCompleted;
using testing::kLoaded;
using testing::kSourceInfo;
using testing::RecordingListener;

class RecordingPlane final : public VideoPlane {
 public:
  explicit RecordingPlane(std::vector<std::string> &lines) : lines_(lines) {}

  std::string windowId() override {
    return "window_id_dummy";
  }
  bool namesTransport() const override {
    return false;
  }
  void show(Scene &, const PixelRect &, const PixelRect &) override {
    // Where the picture goes is not what these tests are about.
  }
  void hide() override {
    // Nothing is shown.
  }
  void bind(const std::string &mediaId, const VideoStream &video) override {
    lines_.push_back(std::format("bind {} {}x{}", mediaId, video.width, video.height));
  }
  void playingChanged(bool playing) override {
    lines_.push_back(playing ? "playing" : "paused");
  }
  void release() override {
    lines_.push_back("release");
  }

 private:
  std::vector<std::string> &lines_;
};

struct PlaneFixture {
  rnwebos::testing::MainLoopScope mainLoop;
  FakeLunaBus bus;
  RecordingListener listener;
  std::vector<std::string> plane;
  LunaMediaPlayer player{bus, std::make_unique<RecordingPlane>(plane), listener, "com.example.app", 1};

  void loadUntilReady() {
    player.load({.uri = "https://example.com/master.m3u8", .headers = {}});
    bus.answer("load", kLoaded);
    bus.emit(kSourceInfo);
    bus.emit(kLoadCompleted);
  }
};

} // namespace

TEST_CASE_FIXTURE(PlaneFixture, "a plane that cannot name the transport loads an HLS URL without one") {
  player.load({.uri = "https://example.com/master.m3u8", .headers = {}});

  CHECK(bus.payloadOf("load").find("mediaTransportType") == std::string::npos);
  CHECK(bus.payloadOf("load").find(R"("windowId":"window_id_dummy")") != std::string::npos);
}

TEST_CASE_FIXTURE(PlaneFixture, "the plane is told the prepared media and its picture once the pipeline has loaded it") {
  player.load({.uri = "https://example.com/master.m3u8", .headers = {}});
  bus.answer("load", kLoaded);
  bus.emit(kSourceInfo);
  const auto beforeLoaded = plane;

  bus.emit(kLoadCompleted);

  CHECK(beforeLoaded.empty());
  CHECK(plane == std::vector<std::string>{"bind _xyz 1920x1080"});
}

TEST_CASE_FIXTURE(PlaneFixture, "the plane follows the media playing and pausing") {
  loadUntilReady();
  plane.clear();

  bus.emit(R"({"playing":{"mediaId":"_xyz"}})");
  bus.emit(R"({"paused":{"mediaId":"_xyz"}})");

  CHECK(plane == std::vector<std::string>{"playing", "paused"});
}

TEST_CASE_FIXTURE(PlaneFixture, "the plane is released when the media is unloaded") {
  loadUntilReady();
  plane.clear();

  player.unload();

  CHECK(plane == std::vector<std::string>{"release"});
}

TEST_CASE_FIXTURE(PlaneFixture, "the plane is released when the TV takes the pipeline away") {
  loadUntilReady();
  plane.clear();

  bus.emit(R"({"unloadCompleted":{"mediaId":"_xyz"}})");

  CHECK(plane.front() == "release");
}

} // namespace rnwebos::expo
