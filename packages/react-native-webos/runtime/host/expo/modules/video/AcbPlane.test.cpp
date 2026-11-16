#include "AcbPlane.h"

#include "VideoTestSupport.test.h"

#include <doctest/doctest.h>

#include <format>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace rnwebos::expo {

namespace {

using testing::pumpFor;

std::vector<std::string> calls;
long nextHandle = 7;
bool initializes = true;
AcbApi::Callback reportTask = nullptr;
std::string videoData;

class RecordingSink final : public LogSink {
 public:
  void write(LogLevel level, std::string_view message) override {
    lines.emplace_back(level, std::string(message));
  }
  std::vector<std::pair<LogLevel, std::string>> lines;
};

AcbApi recordingApi() {
  AcbApi api;
  api.create = [] { return nextHandle; };
  api.initialize = [](long acb, int playerType, const char *appId, AcbApi::Callback callback) {
    reportTask = callback;
    calls.push_back(std::format("initialize {} {} {} {}", acb, playerType, appId, callback != nullptr));
    return initializes;
  };
  api.setSinkType = [](long acb, int sink) {
    calls.push_back(std::format("sink {} {}", acb, sink));
    return true;
  };
  api.setMediaId = [](long acb, const char *mediaId) {
    calls.push_back(std::format("media {} {}", acb, mediaId));
    return true;
  };
  api.setState = [](long acb, int appState, int playState, long *task) {
    calls.push_back(std::format("state {} {} {} {}", acb, appState, playState, task != nullptr));
    return 1;
  };
  api.setMediaVideoData = [](long acb, const char *json, long *task) {
    const std::string text = json;
    videoData = text;
    calls.push_back(std::format(
        "video data {} {} {} {}",
        acb,
        text.find(R"("context":"_xyz")") != std::string::npos,
        text.find(R"("height":1080,"width":1920)") != std::string::npos,
        task != nullptr));
    return 1;
  };
  api.setDisplayWindow = [](long acb, long x, long y, long width, long height, bool fullScreen, long *task) {
    calls.push_back(std::format("window {} {},{} {}x{} {} {}", acb, x, y, width, height, fullScreen, task != nullptr));
    return 1;
  };
  return api;
}

struct AcbFixture {
  AcbFixture() {
    calls.clear();
    videoData.clear();
    nextHandle = 7;
    initializes = true;
  }

  rnwebos::testing::MainLoopScope mainLoop;
  AcbApi api{recordingApi()};
};

const VideoStream kVideo{.width = 1920, .height = 1080, .fps = 25, .bitrate = 3000000, .codec = "hevc"};

const std::vector<std::string> kBound{
    "media 7 _xyz",
    "state 7 1 1 true",
    "video data 7 true true true",
    "window 7 0,0 1920x1080 true true",
    "state 7 1 3 true",
};

} // namespace

TEST_CASE_FIXTURE(AcbFixture, "the control block starts once, as the plain video player on the main sink") {
  AcbSession session(api, "com.example.app");

  CHECK(session.handle() == 7);
  CHECK(calls == std::vector<std::string>{"initialize 7 0 com.example.app true", "sink 7 0"});
}

TEST_CASE_FIXTURE(AcbFixture, "what the control block reports from its own thread is logged, a failure as a warning") {
  AcbSession session(api, "com.example.app");
  auto sink = std::make_shared<RecordingSink>();
  setLogSink(sink);

  std::thread([] {
    reportTask(7, 5, -1, 1, 1, R"({"errorCode":-1})");
    reportTask(7, 6, 1, 1, 2, nullptr);
    reportTask(7, 7, 0, 1, 2, "");
    reportTask(7, 8, -2, 1, 2, "");
    reportTask(7, 9, 4, 1, 2, "");
  }).join();
  setLogSink(nullptr);

  REQUIRE(sink->lines.size() == 5);
  CHECK(sink->lines[0].first == LogLevel::Warning);
  CHECK(sink->lines[0].second == R"([video] control block task 5 failed, play state 1: {"errorCode":-1})");
  CHECK(sink->lines[1].first == LogLevel::Debug);
  CHECK(sink->lines[1].second == "[video] control block task 6 done, play state 2: ");
  CHECK(sink->lines[2].second == "[video] control block task 7 cancelled, play state 2: ");
  CHECK(sink->lines[3].second == "[video] control block task 8 timed out, play state 2: ");
  CHECK(sink->lines[4].second == "[video] control block task 9 answered, play state 2: ");
}

TEST_CASE_FIXTURE(AcbFixture, "a TV that makes no control block starts none") {
  nextHandle = 0;

  AcbSession session(api, "com.example.app");

  CHECK(session.handle() == 0);
  CHECK(calls.empty());
}

TEST_CASE_FIXTURE(AcbFixture, "a control block that refuses to start binds nothing") {
  initializes = false;
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  calls.clear();

  plane.bind("_xyz", kVideo);
  pumpFor(std::chrono::milliseconds(900));
  plane.playingChanged(true);
  plane.release();

  CHECK(session.handle() == 0);
  CHECK(calls.empty());
}

TEST_CASE_FIXTURE(AcbFixture, "a webOS 4 load names a placeholder window and no transport") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);

  CHECK(plane.windowId() == "window_id_dummy");
  CHECK_FALSE(plane.namesTransport());
}

TEST_CASE_FIXTURE(AcbFixture, "a prepared media is bound in LG's order, paced, and waits paused") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  calls.clear();

  plane.bind("_xyz", kVideo);
  pumpFor(std::chrono::milliseconds(100));
  const auto first = calls;
  pumpFor(std::chrono::milliseconds(800));

  CHECK(first == std::vector<std::string>{"media 7 _xyz"});
  CHECK(calls == kBound);
}

TEST_CASE_FIXTURE(AcbFixture, "playing a bound media plays the plane and places the window again") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  plane.bind("_xyz", kVideo);
  pumpFor(std::chrono::milliseconds(900));
  calls.clear();

  plane.playingChanged(true);
  plane.playingChanged(false);

  CHECK(calls == std::vector<std::string>{"state 7 1 2 true", "window 7 0,0 1920x1080 true true", "state 7 1 3 true"});
}

TEST_CASE_FIXTURE(AcbFixture, "a media that plays before the bind ends is bound playing") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);

  plane.bind("_xyz", kVideo);
  plane.playingChanged(true);
  pumpFor(std::chrono::milliseconds(900));

  CHECK(calls.back() == "state 7 1 2 true");
}

TEST_CASE_FIXTURE(AcbFixture, "a release in the middle of a bind stops it and unbinds the plane") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  calls.clear();

  plane.bind("_xyz", kVideo);
  pumpFor(std::chrono::milliseconds(250));
  plane.release();
  pumpFor(std::chrono::milliseconds(800));

  CHECK(calls == std::vector<std::string>{"media 7 _xyz", "state 7 1 1 true", "state 7 1 0 true"});
}

TEST_CASE_FIXTURE(AcbFixture, "a release with nothing bound sends nothing") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  calls.clear();

  plane.release();

  CHECK(calls.empty());
}

TEST_CASE_FIXTURE(AcbFixture, "the plane stays full screen whatever rectangle a view asks for") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  plane.bind("_xyz", kVideo);
  pumpFor(std::chrono::milliseconds(900));
  calls.clear();
  Scene scene{PixelSize{1920, 1080}};

  plane.show(scene, {}, PixelRect{.x = 100, .y = 50, .width = 640, .height = 360});
  plane.hide();

  CHECK(calls.empty());
}

TEST_CASE_FIXTURE(AcbFixture, "the video data names the picture the pipeline described") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);

  plane.bind("_xyz", kVideo);
  pumpFor(std::chrono::milliseconds(900));

  CHECK(videoData.find(R"("bitRate":3000000,)") != std::string::npos);
  CHECK(videoData.find(R"("frameRate":25.0,"hdrType":"none","height":1080,"width":1920,)") != std::string::npos);
  CHECK(videoData.find(R"("mediaVui":{"colorPrimaries":2,)") != std::string::npos);
}

TEST_CASE_FIXTURE(AcbFixture, "what the pipeline left out of the picture is described as 24 fps at 8 Mbps") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);

  plane.bind("_xyz", {.width = 1280, .height = 720, .fps = 0, .bitrate = 0, .codec = "h264"});
  pumpFor(std::chrono::milliseconds(900));
  const auto noRate = videoData;
  plane.bind("_xyz", {.width = 1280, .height = 720, .fps = 30, .bitrate = 0, .codec = "h264"});
  pumpFor(std::chrono::milliseconds(900));
  const auto noBitRate = videoData;
  plane.bind("_xyz", {.width = 1280, .height = 0, .fps = 30, .bitrate = 2000000, .codec = "h264"});
  pumpFor(std::chrono::milliseconds(900));

  CHECK(noRate.find(R"("bitRate":8000000,)") != std::string::npos);
  CHECK(noRate.find(R"("frameRate":24.0,"hdrType":"none","height":720,"width":1280,)") != std::string::npos);
  CHECK(noBitRate.find(R"("bitRate":8000000,)") != std::string::npos);
  CHECK(noBitRate.find(R"("frameRate":30.0,)") != std::string::npos);
  CHECK(videoData.find(R"("bitRate":2000000,)") != std::string::npos);
  CHECK(videoData.find(R"("height":1080,"width":1920,)") != std::string::npos);
}

TEST_CASE_FIXTURE(AcbFixture, "a picture the pipeline did not size is described as 1080p") {
  AcbSession session(api, "com.example.app");
  AcbPlane plane(session, 1);
  calls.clear();

  plane.bind("_xyz", {});
  pumpFor(std::chrono::milliseconds(900));

  CHECK(calls == kBound);
}

} // namespace rnwebos::expo
