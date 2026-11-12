#include "PlayerLog.h"
#include "PlayerModel.h"
#include "PlayerThread.h"

#include "threading/MainLoop.test.h"

#include <doctest/doctest.h>

#include <future>
#include <memory>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace rntizen::expo {

namespace {

class RecordingSink final : public LogSink {
 public:
  void write(LogLevel level, std::string_view message) override {
    lines.emplace_back(level, std::string(message));
  }
  std::vector<std::pair<LogLevel, std::string>> lines;
};

} // namespace

TEST_CASE("playerErrorName names every capi-media-player error and spells out an unknown one") {
  const std::vector<std::pair<int, const char *>> names{
      {PLAYER_ERROR_NONE, "PLAYER_ERROR_NONE"},
      {PLAYER_ERROR_OUT_OF_MEMORY, "PLAYER_ERROR_OUT_OF_MEMORY"},
      {PLAYER_ERROR_INVALID_PARAMETER, "PLAYER_ERROR_INVALID_PARAMETER"},
      {PLAYER_ERROR_NO_SUCH_FILE, "PLAYER_ERROR_NO_SUCH_FILE"},
      {PLAYER_ERROR_INVALID_OPERATION, "PLAYER_ERROR_INVALID_OPERATION"},
      {PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE, "PLAYER_ERROR_FEATURE_NOT_SUPPORTED_ON_DEVICE"},
      {PLAYER_ERROR_SEEK_FAILED, "PLAYER_ERROR_SEEK_FAILED"},
      {PLAYER_ERROR_INVALID_STATE, "PLAYER_ERROR_INVALID_STATE"},
      {PLAYER_ERROR_NOT_SUPPORTED_FILE, "PLAYER_ERROR_NOT_SUPPORTED_FILE"},
      {PLAYER_ERROR_INVALID_URI, "PLAYER_ERROR_INVALID_URI"},
      {PLAYER_ERROR_SOUND_POLICY, "PLAYER_ERROR_SOUND_POLICY"},
      {PLAYER_ERROR_CONNECTION_FAILED, "PLAYER_ERROR_CONNECTION_FAILED"},
      {PLAYER_ERROR_RESOURCE_LIMIT, "PLAYER_ERROR_RESOURCE_LIMIT"},
      {PLAYER_ERROR_PERMISSION_DENIED, "PLAYER_ERROR_PERMISSION_DENIED"},
      {PLAYER_ERROR_SERVICE_DISCONNECTED, "PLAYER_ERROR_SERVICE_DISCONNECTED"},
      {PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC, "PLAYER_ERROR_NOT_SUPPORTED_AUDIO_CODEC"},
      {PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC, "PLAYER_ERROR_NOT_SUPPORTED_VIDEO_CODEC"},
      {PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE, "PLAYER_ERROR_NOT_SUPPORTED_SUBTITLE"},
      {PLAYER_ERROR_NOT_SUPPORTED_FORMAT, "PLAYER_ERROR_NOT_SUPPORTED_FORMAT"},
      {PLAYER_ERROR_NOT_AVAILABLE, "PLAYER_ERROR_NOT_AVAILABLE"},
  };
  for (const auto &[code, name] : names) CHECK(playerErrorName(code) == name);
  CHECK(playerErrorName(0x1234) == "player error 0x1234");
}

TEST_CASE("playerStateName names each player state and NONE for anything else") {
  CHECK(std::string_view(playerStateName(PLAYER_STATE_IDLE)) == "IDLE");
  CHECK(std::string_view(playerStateName(PLAYER_STATE_READY)) == "READY");
  CHECK(std::string_view(playerStateName(PLAYER_STATE_PLAYING)) == "PLAYING");
  CHECK(std::string_view(playerStateName(PLAYER_STATE_PAUSED)) == "PAUSED");
  CHECK(std::string_view(playerStateName(PLAYER_STATE_NONE)) == "NONE");
}

TEST_CASE("decimal and millisSince write a point and the digits asked for") {
  CHECK(decimal(1.23456) == "1.235");
  CHECK(decimal(2, 0) == "2");
  CHECK(decimal(-0.5, 2) == "-0.50");
  const auto elapsed = millisSince(std::chrono::steady_clock::now());
  CHECK(elapsed.ends_with(" ms"));
  CHECK(elapsed.find('.') != std::string::npos);
}

TEST_CASE("threadDescription says whether the caller is on the main loop") {
  const rntizen::testing::MainLoopScope mainLoop;
  CHECK(threadDescription().starts_with("on the main loop (thread "));
  std::string other;
  std::thread([&other] { other = threadDescription(); }).join();
  CHECK(other.starts_with("off the main loop (thread "));
}

TEST_CASE("playerLog prefixes the player's number, at the level asked for") {
  auto sink = std::make_shared<RecordingSink>();
  setLogSink(sink);
  playerLog(7, "prepared");
  playerLog(8, "refused", LogLevel::Error);
  setLogSink(nullptr);
  REQUIRE(sink->lines.size() == 2);
  CHECK(sink->lines[0] == std::pair(LogLevel::Info, std::string("[video 7] prepared")));
  CHECK(sink->lines[1] == std::pair(LogLevel::Error, std::string("[video 8] refused")));
}

TEST_CASE("statusName and contentFitFrom spell expo-video's names") {
  CHECK(std::string_view(statusName(PlayerStatus::Idle)) == "idle");
  CHECK(std::string_view(statusName(PlayerStatus::Loading)) == "loading");
  CHECK(std::string_view(statusName(PlayerStatus::ReadyToPlay)) == "readyToPlay");
  CHECK(std::string_view(statusName(PlayerStatus::Error)) == "error");
  CHECK(std::string_view(statusName(static_cast<PlayerStatus>(42))) == "idle");

  CHECK(contentFitFrom("cover") == ContentFit::Cover);
  CHECK(contentFitFrom("fill") == ContentFit::Fill);
  CHECK(contentFitFrom("contain") == ContentFit::Contain);
  CHECK(contentFitFrom("scale-down") == ContentFit::Contain);
}

TEST_CASE("VideoStream compares every field") {
  const VideoStream stream{.width = 1920, .height = 1080, .fps = 25, .bitrate = 1, .codec = "h264"};
  CHECK(stream == VideoStream{.width = 1920, .height = 1080, .fps = 25, .bitrate = 1, .codec = "h264"});
  CHECK_FALSE(stream == VideoStream{.width = 1920, .height = 1080, .fps = 25, .bitrate = 1, .codec = "hevc"});
}

TEST_CASE("runOnPlayerThread runs every task on one other thread, in order") {
  std::vector<int> order;
  std::promise<std::thread::id> first;
  std::promise<std::thread::id> second;
  runOnPlayerThread([&] {
    order.push_back(1);
    first.set_value(std::this_thread::get_id());
  });
  runOnPlayerThread([&] {
    order.push_back(2);
    second.set_value(std::this_thread::get_id());
  });
  const auto firstThread = first.get_future().get();
  const auto secondThread = second.get_future().get();
  CHECK(firstThread == secondThread);
  CHECK(firstThread != std::this_thread::get_id());
  CHECK(order == std::vector<int>{1, 2});
}

} // namespace rntizen::expo
