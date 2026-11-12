#include "VoiceCommand.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("parseVoiceCommand reads a transport command and its payload") {
  const auto command = parseVoiceCommand(R"({"type":"skip","seconds":-10})");

  REQUIRE(command.has_value());
  CHECK(command->isTransport());
  CHECK(command->payload() == folly::dynamic::object("type", "skip")("seconds", -10.0));
}

TEST_CASE("parseVoiceCommand reads a selection by index, counted from the end when negative") {
  const auto second = parseVoiceCommand(R"({"type":"selectIndex","index":2})");
  const auto last = parseVoiceCommand(R"({"type":"selectIndex","index":-1})");

  REQUIRE(second.has_value());
  REQUIRE(last.has_value());
  CHECK(second->index == 2);
  CHECK(last->index == -1);
  CHECK_FALSE(second->isTransport());
}

TEST_CASE("parseVoiceCommand drops an index that is infinite or outside int's range") {
  CHECK_FALSE(parseVoiceCommand(R"({"type":"selectIndex","index":1e400})").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"selectIndex","index":-1e400})").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"selectIndex","index":1e10})").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"selectIndex","index":-3e9})").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"selectIndex","index":"2"})").has_value());
}

TEST_CASE("parseVoiceCommand fills a search's utterance from its title") {
  const auto command = parseVoiceCommand(R"({"type":"search","title":"Dune","genre":"","cast":7})");

  REQUIRE(command.has_value());
  CHECK(command->payload() == folly::dynamic::object("title", "Dune")("utterance", "Dune"));
}

TEST_CASE("parseVoiceCommand reads subtitles and a title selection") {
  const auto subtitles = parseVoiceCommand(R"({"type":"subtitles","enabled":true})");
  const auto title = parseVoiceCommand(R"({"type":"selectTitle","title":"Dune"})");

  REQUIRE(subtitles.has_value());
  REQUIRE(title.has_value());
  CHECK(subtitles->payload() == folly::dynamic::object("type", "subtitles")("enabled", true));
  CHECK(title->title == "Dune");
}

TEST_CASE("parseVoiceCommand drops what is not a command") {
  CHECK_FALSE(parseVoiceCommand("not json").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"skip","seconds":99999999999999999999})").has_value());
  CHECK_FALSE(parseVoiceCommand("[]").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"dance"})").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"seek"})").has_value());
  CHECK_FALSE(parseVoiceCommand(R"({"type":"subtitles","enabled":1})").has_value());
}

} // namespace rntizen
