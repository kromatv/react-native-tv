#include "ContentContext.h"

#include "VoiceScreen.test.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace rntizen {

namespace {

using facebook::react::Rect;
using testing::SentTouch;
using testing::VoiceScreen;

constexpr facebook::react::Float kTwoThirds = 2.0F / 3.0F;

LabelledView labelled(const char *label, facebook::react::Float x, facebook::react::Float y) {
  return {.view = nullptr, .label = label, .frame = {.origin = {.x = x, .y = y}, .size = {.width = 200, .height = 100}}};
}

Rect frame(facebook::react::Float x, facebook::react::Float y, facebook::react::Float width = 200, facebook::react::Float height = 100) {
  return {.origin = {.x = x, .y = y}, .size = {.width = width, .height = height}};
}

std::vector<std::string> labels(const std::vector<LabelledView> &views) {
  std::vector<std::string> names(views.size());
  std::ranges::transform(views, names.begin(), &LabelledView::label);
  return names;
}

} // namespace

TEST_CASE("findByTitle prefers an exact label, then a prefix, then a part of one") {
  const std::vector views{
      labelled("The Dune Saga", 0, 0), labelled("Dune: Part Two", 200, 0), labelled("dune", 400, 0)};

  CHECK(findByTitle(views, "DUNE")->label == "dune");
  CHECK(findByTitle(views, "dune part")->label == "Dune: Part Two");
  CHECK(findByTitle(views, "saga")->label == "The Dune Saga");
}

TEST_CASE("findByTitle finds nothing for a title with no letters or no match") {
  const std::vector views{labelled("Dune", 0, 0)};

  CHECK(findByTitle(views, "?!") == nullptr);
  CHECK(findByTitle(views, "Arrival") == nullptr);
}

TEST_CASE("findByTitle ignores leading punctuation and keeps letters outside ASCII as they are") {
  const std::vector views{labelled("Am\xC3\xA9lie", 0, 0), labelled("...Leon", 200, 0)};

  CHECK(findByTitle(views, "  am\xC3\xA9lie!")->label == "Am\xC3\xA9lie");
  CHECK(findByTitle(views, "leon")->label == "...Leon");
}

TEST_CASE("contentContext places each title at its view's center") {
  const std::vector views{labelled("Dune", 100, 50)};

  const auto items = contentContext(views);

  REQUIRE(items.size() == 1);
  CHECK(items[0]["positionX"] == 200);
  CHECK(items[0]["positionY"] == 100);
  CHECK(items[0]["title"] == "Dune");
  CHECK(items[0]["alias"].empty());
  CHECK(items[0]["bFocused"] == false);
}

TEST_CASE("labelledViews lists the labelled views top to bottom, then left to right within a row") {
  VoiceScreen screen;
  screen.add({.label = "Right", .frame = frame(400, 0)});
  screen.add({.label = "Below", .frame = frame(0, 300)});
  screen.add({.label = "Left", .frame = frame(0, 5)});

  const auto views = labelledViews(screen.root());

  CHECK(labels(views) == std::vector<std::string>{"Left", "Right", "Below"});
}

TEST_CASE("labelledViews finds a label inside an unlabelled view but not inside a labelled one") {
  VoiceScreen screen;
  auto &row = screen.add({.label = "", .frame = frame(0, 400, 1920, 300)});
  auto &card = screen.add(row, {.label = "Dune", .frame = frame(100, 50)});
  screen.add(card, {.label = "Play", .frame = frame(10, 10, 50, 50)});

  const auto views = labelledViews(screen.root());

  REQUIRE(views.size() == 1);
  CHECK(views[0].view == &card);
  CHECK(views[0].label == "Dune");
  CHECK(views[0].frame == frame(100, 450));
}

TEST_CASE("labelledViews skips a view that is hidden, transparent, without props or empty") {
  VoiceScreen screen;
  screen.add({.label = "Shown", .frame = frame(0, 0)});
  screen.add({.label = "Transparent", .frame = frame(0, 200), .opacity = 0});
  screen.add({.label = "Hidden", .frame = frame(0, 400), .display = facebook::react::DisplayType::None});
  screen.add({.label = "No width", .frame = frame(0, 600, 0, 100)});
  screen.add({.label = "No height", .frame = frame(0, 800, 200, 0)});
  auto &faded = screen.add({.label = "", .frame = frame(400, 0, 400, 400), .opacity = 0});
  screen.add(faded, {.label = "Inside a transparent view", .frame = frame(0, 0)});
  screen.addWithoutProps();

  CHECK(labels(labelledViews(screen.root())) == std::vector<std::string>{"Shown"});
}

TEST_CASE("labelledViews skips a view outside the logical screen on any side") {
  VoiceScreen screen;
  screen.add({.label = "Right of it", .frame = frame(1920, 0)});
  screen.add({.label = "Below it", .frame = frame(0, 1080)});
  screen.add({.label = "Left of it", .frame = frame(-200, 0)});
  screen.add({.label = "Above it", .frame = frame(0, -100)});
  screen.add({.label = "Half in", .frame = frame(1820, 1030)});

  CHECK(labels(labelledViews(screen.root())) == std::vector<std::string>{"Half in"});
}

TEST_CASE("labelledViews measures in points whatever the window's scale") {
  VoiceScreen scaled(kTwoThirds);
  VoiceScreen unscaled(0);
  scaled.add({.label = "Dune", .frame = frame(300, 150)});
  unscaled.add({.label = "Dune", .frame = frame(300, 150)});

  const auto inScaled = labelledViews(scaled.root());
  const auto inUnscaled = labelledViews(unscaled.root());

  REQUIRE(inScaled.size() == 1);
  CHECK(inScaled[0].frame.origin.x == doctest::Approx(300));
  CHECK(inScaled[0].frame.origin.y == doctest::Approx(150));
  REQUIRE(inUnscaled.size() == 1);
  CHECK(inUnscaled[0].frame == frame(300, 150));
}

TEST_CASE("tapView touches the view at its center, a start then an end") {
  VoiceScreen screen;
  const auto &card = screen.add({.label = "Dune", .frame = frame(100, 50)});
  const auto views = labelledViews(screen.root());
  REQUIRE(views.size() == 1);

  CHECK(tapView(views[0]));

  CHECK(
      screen.touches() ==
      std::vector<SentTouch>{
          {.type = "topTouchStart", .target = card.tag(), .pageX = 200, .pageY = 100},
          {.type = "topTouchEnd", .target = card.tag(), .pageX = 200, .pageY = 100},
      });
}

TEST_CASE("tapView cannot press a view that sends no touch events") {
  VoiceScreen screen;
  screen.add({.label = "Poster", .frame = frame(0, 0), .touchable = false});
  const auto views = labelledViews(screen.root());
  REQUIRE(views.size() == 1);

  CHECK_FALSE(tapView(views[0]));
  CHECK(screen.touches().empty());
}

} // namespace rntizen
