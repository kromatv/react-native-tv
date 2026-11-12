#include "PlayerLog.h"
#include "PlayerModel.h"

#include <doctest/doctest.h>

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace rnwebos::expo {

namespace {

class RecordingSink final : public LogSink {
 public:
  void write(LogLevel level, std::string_view message) override {
    lines.emplace_back(level, std::string(message));
  }
  std::vector<std::pair<LogLevel, std::string>> lines;
};

} // namespace

TEST_CASE("decimal writes a point and the digits asked for") {
  CHECK(decimal(1.23456) == "1.235");
  CHECK(decimal(2, 0) == "2");
  CHECK(decimal(-0.5, 2) == "-0.50");
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

} // namespace rnwebos::expo
