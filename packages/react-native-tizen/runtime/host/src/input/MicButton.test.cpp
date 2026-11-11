#include "MicButton.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

namespace rntizen {

namespace {

struct Harness {
  std::vector<bool> grabs;
  std::vector<std::string> presses;
  std::shared_ptr<MicButton> button = std::make_shared<MicButton>(
      [this](std::string_view type) { presses.emplace_back(type); },
      [](std::function<void()> task) { task(); });

  Harness() {
    button->setGrab([this](bool grabbed) { grabs.push_back(grabbed); });
  }
};

} // namespace

TEST_CASE("a listener grabs the mic button, and its presses arrive as they happen") {
  Harness mic;

  mic.button->claim();
  const bool down = mic.button->handleKey(kMicButtonKey, true);
  const bool repeat = mic.button->handleKey(kMicButtonKey, true);
  const bool up = mic.button->handleKey(kMicButtonKey, false);

  CHECK(mic.grabs == std::vector<bool>{true});
  CHECK(down);
  CHECK(repeat);
  CHECK(up);
  CHECK(mic.presses == std::vector<std::string>{"pressIn", "pressOut"});
  CHECK_FALSE(mic.button->handleKey("Return", true));
}

TEST_CASE("without a listener the mic button is not grabbed and its key goes on to the remote") {
  Harness mic;

  CHECK_FALSE(mic.button->handleKey(kMicButtonKey, true));
  CHECK(mic.grabs.empty());
  CHECK(mic.presses.empty());
}

TEST_CASE("the last listener gone gives the button back and ends a held press") {
  Harness mic;
  mic.button->claim();
  mic.button->claim();
  mic.button->handleKey(kMicButtonKey, true);

  mic.button->unclaim();
  CHECK(mic.grabs == std::vector<bool>{true});
  mic.button->unclaim();
  mic.button->unclaim();

  CHECK(mic.grabs == std::vector<bool>{true, false});
  CHECK(mic.presses == std::vector<std::string>{"pressIn", "pressOut"});
}

TEST_CASE("a pause ends a held press, and a reload forgets every listener") {
  Harness mic;
  mic.button->claim();
  mic.button->handleKey(kMicButtonKey, true);

  mic.button->releaseHeld();
  mic.button->releaseHeld();
  mic.button->reset();

  CHECK(mic.presses == std::vector<std::string>{"pressIn", "pressOut"});
  CHECK(mic.grabs == std::vector<bool>{true, false});
}

TEST_CASE("listeners that came before the window are honoured once it exists") {
  std::vector<bool> grabs;
  const auto button = std::make_shared<MicButton>([](std::string_view) {}, [](std::function<void()> task) { task(); });

  button->claim();
  button->setGrab([&grabs](bool grabbed) { grabs.push_back(grabbed); });

  CHECK(grabs == std::vector<bool>{true});
}

} // namespace rntizen
