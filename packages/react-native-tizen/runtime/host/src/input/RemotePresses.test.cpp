#include "RemotePresses.h"

#include <doctest/doctest.h>

#include <Ecore.h>

#include <algorithm>
#include <string>
#include <vector>

namespace rntizen {

namespace {

constexpr unsigned int kReturn = 36;
constexpr unsigned int kUp = 111;
constexpr unsigned int kPlay = 172;
constexpr double kPastHoldDelay = 0.4;

Ecore_Event_Key keyEvent(const char *name, unsigned int keycode, unsigned int timestamp = 0) {
  Ecore_Event_Key key{};
  key.keyname = name;
  key.key = name;
  key.keycode = keycode;
  key.timestamp = timestamp;
  return key;
}

void runMainLoopFor(double seconds) {
  ecore_timer_add(
      seconds,
      [](void *) -> Eina_Bool {
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
      },
      nullptr);
  ecore_main_loop_begin();
}

std::string describe(const RemoteEvent &event) {
  return std::string(event.eventType) + "/" + std::to_string(event.keyAction) + (event.isBack ? " back" : "");
}

struct EcoreLoop {
  EcoreLoop() {
    ecore_init();
  }
  ~EcoreLoop() {
    ecore_shutdown();
  }
  EcoreLoop(const EcoreLoop &) = delete;
  EcoreLoop &operator=(const EcoreLoop &) = delete;
};

struct Remote {
  EcoreLoop loop;
  std::vector<std::string> events;
  bool fieldTakesKeys{false};
  RemotePresses presses{
      [this](const RemoteEvent &event) { events.push_back(describe(event)); },
      [this](const Ecore_Event_Key &, bool) { return fieldTakesKeys; }};
};

} // namespace

TEST_CASE_FIXTURE(Remote, "a key reports its press at its release") {
  CHECK(presses.keyDown(keyEvent("Return", kReturn)));
  CHECK(events.empty());

  CHECK(presses.keyUp(keyEvent("Return", kReturn)));

  CHECK(events == std::vector<std::string>{"select/1"});
}

TEST_CASE_FIXTURE(Remote, "Back reports one press, marked as Back, at its release") {
  presses.keyDown(keyEvent("XF86Back", 166));

  presses.keyUp(keyEvent("XF86Back", 166));

  CHECK(events == std::vector<std::string>{"back/1 back"});
}

TEST_CASE_FIXTURE(Remote, "the TV's repeats of a held key are not presses") {
  presses.keyDown(keyEvent("XF86AudioPlay", kPlay, 0));
  presses.keyDown(keyEvent("XF86AudioPlay", kPlay, 100));
  presses.keyDown(keyEvent("XF86AudioPlay", kPlay, 200));

  presses.keyUp(keyEvent("XF86AudioPlay", kPlay, 250));

  CHECK(events == std::vector<std::string>{"play/1"});
}

TEST_CASE_FIXTURE(Remote, "a key down again after a second of silence lost its release and starts again") {
  presses.keyDown(keyEvent("XF86AudioPlay", kPlay, 0));

  presses.keyDown(keyEvent("XF86AudioPlay", kPlay, 1500));
  presses.keyUp(keyEvent("XF86AudioPlay", kPlay, 1600));

  CHECK(events == std::vector<std::string>{"play/1"});
  CHECK_FALSE(presses.keyUp(keyEvent("XF86AudioPlay", kPlay, 1700)));
}

TEST_CASE_FIXTURE(Remote, "a key held down starts a hold, and its release ends it") {
  presses.keyDown(keyEvent("Return", kReturn));

  runMainLoopFor(kPastHoldDelay);
  presses.keyUp(keyEvent("Return", kReturn));

  CHECK(events == std::vector<std::string>{"longSelect/0", "longSelect/1"});
}

TEST_CASE_FIXTURE(Remote, "a release the key filter uses ends the hold its key started") {
  presses.keyDown(keyEvent("Return", kReturn));
  runMainLoopFor(kPastHoldDelay);
  fieldTakesKeys = true;

  const bool used = presses.keyUp(keyEvent("Return", kReturn));

  CHECK(used);
  CHECK(events == std::vector<std::string>{"longSelect/0", "longSelect/1"});
  fieldTakesKeys = false;
  CHECK_FALSE(presses.keyUp(keyEvent("Return", kReturn)));
}

TEST_CASE_FIXTURE(Remote, "a release the key filter uses is not a press, and no hold starts after it") {
  presses.keyDown(keyEvent("Return", kReturn));
  fieldTakesKeys = true;

  presses.keyUp(keyEvent("Return", kReturn));
  runMainLoopFor(kPastHoldDelay);

  CHECK(events.empty());
}

TEST_CASE_FIXTURE(Remote, "a key the filter uses is never a remote key") {
  fieldTakesKeys = true;

  CHECK(presses.keyDown(keyEvent("Return", kReturn)));
  fieldTakesKeys = false;

  CHECK_FALSE(presses.keyUp(keyEvent("Return", kReturn)));
  CHECK(events.empty());
}

TEST_CASE_FIXTURE(Remote, "a key with no remote meaning or no symbol is left to others") {
  auto unnamed = keyEvent("Return", kReturn);
  unnamed.key = nullptr;

  CHECK_FALSE(presses.keyDown(keyEvent("F13", 191)));
  CHECK_FALSE(presses.keyDown(unnamed));

  CHECK_FALSE(presses.keyUp(keyEvent("F13", 191)));
  CHECK(events.empty());
}

TEST_CASE_FIXTURE(Remote, "releaseAll ends every hold and reports no press") {
  presses.keyDown(keyEvent("Return", kReturn));
  presses.keyDown(keyEvent("Up", kUp));
  runMainLoopFor(kPastHoldDelay);
  presses.keyDown(keyEvent("XF86AudioPlay", kPlay));

  presses.releaseAll();

  std::ranges::sort(events);
  CHECK(events == std::vector<std::string>{"longSelect/0", "longSelect/1", "longUp/0", "longUp/1"});
  CHECK_FALSE(presses.keyUp(keyEvent("Return", kReturn)));
}

TEST_CASE("a hold not started when the presses are destroyed never starts") {
  EcoreLoop loop;
  std::vector<std::string> events;
  {
    RemotePresses presses([&events](const RemoteEvent &event) { events.push_back(describe(event)); });
    presses.keyDown(keyEvent("Return", kReturn));
  }

  runMainLoopFor(kPastHoldDelay);

  CHECK(events.empty());
}

} // namespace rntizen
