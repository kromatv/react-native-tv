#include "RemotePresses.h"

#include "../threading/MainLoop.test.h"

#include <doctest/doctest.h>

#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

constexpr auto kReturn = SDL_SCANCODE_RETURN;
constexpr auto kUp = SDL_SCANCODE_UP;
constexpr auto kPlay = SDL_SCANCODE_MEDIA_PLAY;
constexpr auto kStop = SDL_SCANCODE_MEDIA_STOP;
constexpr auto kBack = SDL_SCANCODE_WEBOS_BACK;
constexpr Uint16 kRawOk = 352;
constexpr auto kPastHoldDelay = std::chrono::milliseconds(400);

SDL_KeyboardEvent keyEvent(SDL_Scancode scancode, Uint64 milliseconds = 0, Uint16 raw = 0) {
  SDL_KeyboardEvent key{};
  key.scancode = scancode;
  key.raw = raw;
  key.timestamp = milliseconds * 1'000'000;
  return key;
}

std::string describe(const RemoteEvent &event) {
  return std::string(event.eventType) + "/" + std::to_string(event.keyAction) + (event.isBack ? " back" : "");
}

struct Remote {
  testing::MainLoopScope loop;
  std::vector<std::string> events;
  bool fieldTakesKeys{false};
  RemotePresses presses{
      [this](const RemoteEvent &event) { events.push_back(describe(event)); },
      [this](const SDL_KeyboardEvent &, bool) { return fieldTakesKeys; }};
};

} // namespace

TEST_CASE_FIXTURE(Remote, "a key reports its press at its release") {
  CHECK(presses.keyDown(keyEvent(kReturn)));
  CHECK(events.empty());

  CHECK(presses.keyUp(keyEvent(kReturn)));

  CHECK(events == std::vector<std::string>{"select/1"});
}

TEST_CASE_FIXTURE(Remote, "Back reports one press, marked as Back, at its release") {
  presses.keyDown(keyEvent(kBack));

  presses.keyUp(keyEvent(kBack));

  CHECK(events == std::vector<std::string>{"back/1 back"});
}

TEST_CASE_FIXTURE(Remote, "the TV's repeats of a held key are not presses") {
  presses.keyDown(keyEvent(kPlay, 0));
  presses.keyDown(keyEvent(kPlay, 100));
  presses.keyDown(keyEvent(kPlay, 200));

  presses.keyUp(keyEvent(kPlay, 250));

  CHECK(events == std::vector<std::string>{"play/1"});
}

TEST_CASE_FIXTURE(Remote, "a key down again after a second of silence lost its release and starts again") {
  presses.keyDown(keyEvent(kPlay, 0));

  presses.keyDown(keyEvent(kPlay, 1500));
  presses.keyUp(keyEvent(kPlay, 1600));

  CHECK(events == std::vector<std::string>{"play/1"});
  CHECK_FALSE(presses.keyUp(keyEvent(kPlay, 1700)));
}

TEST_CASE_FIXTURE(Remote, "a key held down starts a hold, and its release ends it") {
  presses.keyDown(keyEvent(kReturn));

  loop.runFor(kPastHoldDelay);
  presses.keyUp(keyEvent(kReturn));

  CHECK(events == std::vector<std::string>{"longSelect/0", "longSelect/1"});
}

TEST_CASE_FIXTURE(Remote, "a release the key filter uses ends the hold its key started") {
  presses.keyDown(keyEvent(kReturn));
  loop.runFor(kPastHoldDelay);
  fieldTakesKeys = true;

  const bool used = presses.keyUp(keyEvent(kReturn));

  CHECK(used);
  CHECK(events == std::vector<std::string>{"longSelect/0", "longSelect/1"});
  fieldTakesKeys = false;
  CHECK_FALSE(presses.keyUp(keyEvent(kReturn)));
}

TEST_CASE_FIXTURE(Remote, "a release the key filter uses is not a press, and no hold starts after it") {
  presses.keyDown(keyEvent(kReturn));
  fieldTakesKeys = true;

  presses.keyUp(keyEvent(kReturn));
  loop.runFor(kPastHoldDelay);

  CHECK(events.empty());
}

TEST_CASE_FIXTURE(Remote, "a key the filter uses is never a remote key") {
  fieldTakesKeys = true;

  CHECK(presses.keyDown(keyEvent(kReturn)));
  fieldTakesKeys = false;

  CHECK_FALSE(presses.keyUp(keyEvent(kReturn)));
  CHECK(events.empty());
}

TEST_CASE_FIXTURE(Remote, "a key with no remote meaning, scancode or known key code is left to others") {
  CHECK_FALSE(presses.keyDown(keyEvent(SDL_SCANCODE_F13)));
  CHECK_FALSE(presses.keyDown(keyEvent(SDL_SCANCODE_UNKNOWN, 0, 999)));

  CHECK_FALSE(presses.keyUp(keyEvent(SDL_SCANCODE_F13)));
  CHECK(events.empty());
}

TEST_CASE_FIXTURE(Remote, "keys SDL has no scancode for are told apart by their key code") {
  presses.keyDown(keyEvent(SDL_SCANCODE_UNKNOWN, 0, kRawOk));
  CHECK_FALSE(presses.keyUp(keyEvent(SDL_SCANCODE_UNKNOWN, 0, 999)));

  CHECK(presses.keyUp(keyEvent(SDL_SCANCODE_UNKNOWN, 0, kRawOk)));

  CHECK(events == std::vector<std::string>{"select/1"});
}

TEST_CASE_FIXTURE(Remote, "releaseAll ends every hold and reports no press") {
  presses.keyDown(keyEvent(kReturn));
  presses.keyDown(keyEvent(kUp));
  loop.runFor(kPastHoldDelay);
  presses.keyDown(keyEvent(kPlay));

  presses.releaseAll();

  std::ranges::sort(events);
  CHECK(events == std::vector<std::string>{"longSelect/0", "longSelect/1", "longUp/0", "longUp/1"});
  CHECK_FALSE(presses.keyUp(keyEvent(kReturn)));
}

TEST_CASE("a hold not started when the presses are destroyed never starts") {
  const testing::MainLoopScope loop;
  std::vector<std::string> events;
  {
    RemotePresses presses([&events](const RemoteEvent &event) { events.push_back(describe(event)); });
    presses.keyDown(keyEvent(kReturn));
  }

  loop.runFor(kPastHoldDelay);

  CHECK(events.empty());
}

TEST_CASE("presses with no key filter report a key at its release, and nothing for a key down when destroyed") {
  const testing::MainLoopScope loop;
  std::vector<std::string> events;
  {
    RemotePresses presses([&events](const RemoteEvent &event) { events.push_back(describe(event)); });
    presses.keyDown(keyEvent(kPlay));
    CHECK(presses.keyUp(keyEvent(kPlay)));
    presses.keyDown(keyEvent(kStop));
  }

  CHECK(events == std::vector<std::string>{"play/1"});
}

} // namespace rnwebos
