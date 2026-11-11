#include "SpeechContract.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen::expo::speech {

TEST_CASE("start's options are read from expo-speech-recognition's fields") {
  const auto options = speechOptionsFrom(
      folly::dynamic::object("lang", "en-US")("interimResults", true)("maxAlternatives", 3)("continuous", true));

  CHECK(options.language == "en-US");
  CHECK(options.interimResults);
  CHECK(options.maxAlternatives == 3);
}

TEST_CASE("start's options keep their defaults where a field is missing or of another type") {
  const auto mistyped = speechOptionsFrom(folly::dynamic::object("lang", 1)("interimResults", "yes")("maxAlternatives", 0));
  const auto empty = speechOptionsFrom(nullptr);

  CHECK(mistyped.language.empty());
  CHECK_FALSE(mistyped.interimResults);
  CHECK(mistyped.maxAlternatives == 1);
  CHECK(speechOptionsFrom(folly::dynamic::object("maxAlternatives", "2")).maxAlternatives == 1);
  CHECK(empty.language.empty());
  CHECK(empty.maxAlternatives == 1);
}

TEST_CASE("each state has the name getStateAsync answers") {
  CHECK(std::string(speechStateName(SpeechState::Inactive)) == "inactive");
  CHECK(std::string(speechStateName(SpeechState::Starting)) == "starting");
  CHECK(std::string(speechStateName(SpeechState::Recognizing)) == "recognizing");
  CHECK(std::string(speechStateName(SpeechState::Stopping)) == "stopping");
}

} // namespace rntizen::expo::speech
