#include "VoiceController.h"

#include "Voice.test.h"
#include "VoiceScreen.test.h"

#include <doctest/doctest.h>
#include <folly/dynamic.h>
#include <folly/json.h>

#include <optional>

namespace rntizen {

namespace {

using facebook::react::Rect;
using testing::Voice;
using testing::VoiceScreen;

Rect frame(facebook::react::Float x, facebook::react::Float y) {
  return {.origin = {.x = x, .y = y}, .size = {.width = 200, .height = 100}};
}

} // namespace

TEST_CASE("VoiceController's content context is every labelled view on screen, as Samsung's JSON") {
  VoiceScreen screen;
  Voice voice(std::nullopt, &screen);
  screen.add({.label = "Dune", .frame = frame(100, 50)});

  const auto context = folly::parseJson(voice.controller.contentContext());

  CHECK(
      context ==
      folly::dynamic::array(folly::dynamic::object("positionX", 200)("positionY", 100)("title", "Dune")(
          "alias", folly::dynamic::array())("bFocused", false)));
}

TEST_CASE("VoiceController's content context is empty before the app mounts its root view") {
  const Voice voice;

  CHECK(voice.controller.contentContext() == "[]");
}

TEST_CASE("VoiceController presses the view whose label the user said") {
  VoiceScreen screen;
  Voice voice(std::nullopt, &screen);
  screen.add({.label = "Arrival", .frame = frame(0, 0)});
  const auto &dune = screen.add({.label = "Dune: Part Two", .frame = frame(200, 0)});

  CHECK(voice.handle(folly::dynamic::object("type", "selectTitle")("title", "dune part two")));

  REQUIRE(screen.touches().size() == 2);
  CHECK(screen.touches()[0].target == dune.tag());
  CHECK(screen.touches()[1].target == dune.tag());
}

TEST_CASE("VoiceController selects by position on screen, 1 for the first and -1 for the last") {
  VoiceScreen screen;
  Voice voice(std::nullopt, &screen);
  const auto &first = screen.add({.label = "Arrival", .frame = frame(0, 0)});
  screen.add({.label = "Dune", .frame = frame(200, 0)});
  const auto &last = screen.add({.label = "Sicario", .frame = frame(400, 0)});

  CHECK(voice.handle(folly::dynamic::object("type", "selectIndex")("index", 1)));
  CHECK(voice.handle(folly::dynamic::object("type", "selectIndex")("index", -1)));

  REQUIRE(screen.touches().size() == 4);
  CHECK(screen.touches()[0].target == first.tag());
  CHECK(screen.touches()[2].target == last.tag());
}

TEST_CASE("VoiceController leaves a selection to the TV when no view on screen answers it") {
  VoiceScreen screen;
  Voice voice(std::nullopt, &screen);
  screen.add({.label = "Dune", .frame = frame(0, 0)});
  screen.add({.label = "Poster", .frame = frame(200, 0), .touchable = false});

  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "selectTitle")("title", "Arrival")));
  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "selectTitle")("title", "Poster")));
  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "selectIndex")("index", 0)));
  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "selectIndex")("index", 3)));
  CHECK_FALSE(voice.handle(folly::dynamic::object("type", "selectIndex")("index", -3)));
  CHECK(screen.touches().empty());
}

TEST_CASE("VoiceController selects nothing on an empty screen or before the root view is mounted") {
  VoiceScreen screen;
  Voice onEmptyScreen(std::nullopt, &screen);
  Voice beforeMount;

  CHECK_FALSE(onEmptyScreen.handle(folly::dynamic::object("type", "selectIndex")("index", 1)));
  CHECK_FALSE(beforeMount.handle(folly::dynamic::object("type", "selectTitle")("title", "Dune")));
}

} // namespace rntizen
