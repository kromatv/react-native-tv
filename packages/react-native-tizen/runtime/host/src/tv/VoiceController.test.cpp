#include "VoiceController.h"

#include "MediaSessions.h"
#include "Voice.test.h"

#include <doctest/doctest.h>
#include <folly/dynamic.h>

#include <format>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

using testing::Voice;

class RecordingSession final : public MediaSession {
 public:
  explicit RecordingSession(bool playing = false, bool hasSubtitles = true)
      : playing_(playing), hasSubtitles_(hasSubtitles) {}

  bool isPlaying() const override {
    return playing_;
  }
  bool staysActiveInBackground() const override {
    return false;
  }
  void play() override {
    calls.emplace_back("play");
  }
  void pause() override {
    calls.emplace_back("pause");
  }
  void restart() override {
    calls.emplace_back("restart");
  }
  void seekBy(double seconds) override {
    calls.push_back(std::format("seekBy {}", seconds));
  }
  void seekTo(double seconds) override {
    calls.push_back(std::format("seekTo {}", seconds));
  }
  bool setSubtitlesEnabled(bool enabled) override {
    calls.push_back(std::format("subtitles {}", enabled));
    return hasSubtitles_;
  }

  std::vector<std::string> calls;

 private:
  bool playing_;
  bool hasSubtitles_;
};

folly::dynamic command(const char *type) {
  return folly::dynamic::object("type", type);
}

} // namespace

TEST_CASE("VoiceController names Samsung's app state for the screen JavaScript shows") {
  Voice voice;
  const std::vector<std::pair<std::string, std::string>> screens{
      {"home", "Home"},
      {"list", "List"},
      {"details", "Details"},
      {"player", "Player"},
      {"search", "Search"},
      {"settings", "Setting"},
  };

  for (const auto &[screen, state] : screens) {
    voice.state->setScreen(screen);
    CHECK(voice.controller.appState() == state);
  }
}

TEST_CASE("VoiceController's app state is Player while a video plays and List otherwise, for any other screen") {
  Voice voice;
  voice.state->setScreen("profile");

  CHECK(voice.controller.appState() == "List");
  const auto idle = std::make_shared<RecordingSession>(false);
  addMediaSession(idle);
  CHECK(voice.controller.appState() == "List");
  const auto playing = std::make_shared<RecordingSession>(true);
  addMediaSession(playing);
  CHECK(voice.controller.appState() == "Player");
  voice.state->setScreen(std::nullopt);
  CHECK(voice.controller.appState() == "Player");
}

TEST_CASE("VoiceController gives a transport command to JavaScript while it listens") {
  Voice voice;
  const auto session = std::make_shared<RecordingSession>(true);
  addMediaSession(session);
  voice.state->addListener("command", 1);

  CHECK(voice.handle(folly::dynamic::object("type", "skip")("seconds", 30)));

  REQUIRE(voice.events.size() == 1);
  CHECK(voice.events[0].name == VoiceController::kCommandEvent);
  CHECK(voice.events[0].payload == folly::dynamic::object("type", "skip")("seconds", 30));
  CHECK(session->calls.empty());
}

TEST_CASE("VoiceController drives the current player when JavaScript does not listen to commands") {
  Voice voice;
  const auto session = std::make_shared<RecordingSession>(true);
  addMediaSession(session);

  CHECK(voice.handle(command("play")));
  CHECK(voice.handle(command("pause")));
  CHECK(voice.handle(command("stop")));
  CHECK(voice.handle(command("restart")));
  CHECK(voice.handle(folly::dynamic::object("type", "skip")("seconds", -10)));
  CHECK(voice.handle(folly::dynamic::object("type", "seek")("position", 4800)));
  CHECK(voice.handle(folly::dynamic::object("type", "subtitles")("enabled", true)));

  CHECK(
      session->calls ==
      std::vector<std::string>{"play", "pause", "pause", "restart", "seekBy -10", "seekTo 4800", "subtitles true"});
  CHECK(voice.events.empty());
}

TEST_CASE("VoiceController leaves next, previous, fast forward and rewind to the TV") {
  Voice voice;
  const auto session = std::make_shared<RecordingSession>(true);
  addMediaSession(session);

  CHECK_FALSE(voice.handle(command("next")));
  CHECK_FALSE(voice.handle(command("previous")));
  CHECK_FALSE(voice.handle(command("fastForward")));
  CHECK_FALSE(voice.handle(command("rewind")));
  CHECK(session->calls.empty());
}

TEST_CASE("VoiceController leaves subtitles to the TV when the media has none to show") {
  Voice voice;
  const auto session = std::make_shared<RecordingSession>(true, false);
  addMediaSession(session);

  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "subtitles")("enabled", false)));
  CHECK(session->calls == std::vector<std::string>{"subtitles false"});
}

TEST_CASE("VoiceController leaves a transport command to the TV with no player and no listener") {
  Voice voice;

  CHECK_FALSE(voice.handle(command("play")));
}

TEST_CASE("VoiceController leaves to the TV what is not a command") {
  Voice voice;
  voice.state->addListener("command", 1);

  CHECK_FALSE(voice.controller.handle("not json"));
  CHECK_FALSE(voice.handle(command("dance")));
  CHECK(voice.events.empty());
}

TEST_CASE("VoiceController gives a search to JavaScript while it listens") {
  Voice voice("myapp://search?q={query}");
  voice.state->addListener("search", 1);

  CHECK(voice.handle(folly::dynamic::object("type", "search")("title", "Blade Runner")("genre", "sci-fi")));

  REQUIRE(voice.events.size() == 1);
  CHECK(voice.events[0].name == VoiceController::kSearchEvent);
  CHECK(
      voice.events[0].payload ==
      folly::dynamic::object("title", "Blade Runner")("genre", "sci-fi")("utterance", "Blade Runner"));
  CHECK(voice.openedUrls.empty());
}

TEST_CASE("VoiceController opens the search URL when JavaScript does not listen to searches") {
  Voice voice("myapp://search?q={query}");

  CHECK(voice.handle(folly::dynamic::object("type", "search")("utterance", "find blade runner")("title", "Blade Runner")));

  CHECK(voice.openedUrls == std::vector<std::string>{"myapp://search?q=Blade%20Runner"});
  CHECK(voice.events.empty());
}

TEST_CASE("VoiceController leaves a search to the TV with no listener and no search URL") {
  Voice voice;

  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "search")("title", "Blade Runner")));
  CHECK(voice.openedUrls.empty());
}

} // namespace rntizen
