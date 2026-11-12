#include "RemoteKeyMap.h"

#include <doctest/doctest.h>

#include <string>
#include <utility>
#include <vector>

namespace rnwebos {

namespace {

std::string eventOf(SDL_Scancode scancode, Uint16 raw = 0) {
  const auto key = remoteKeyFor(scancode, raw);
  return key ? std::string(key->eventType) : "none";
}

} // namespace

TEST_CASE("remoteKeyFor maps the remote's keys to react-native-tvos event types") {
  for (const auto &[scancode, eventType] : std::vector<std::pair<SDL_Scancode, std::string>>{
           {SDL_SCANCODE_UP, "up"},
           {SDL_SCANCODE_DOWN, "down"},
           {SDL_SCANCODE_LEFT, "left"},
           {SDL_SCANCODE_RIGHT, "right"},
           {SDL_SCANCODE_RETURN, "select"},
           {SDL_SCANCODE_KP_ENTER, "select"},
           {SDL_SCANCODE_MEDIA_PLAY_PAUSE, "playPause"},
           {SDL_SCANCODE_MEDIA_PLAY, "play"},
           {SDL_SCANCODE_MEDIA_PAUSE, "pause"},
           {SDL_SCANCODE_MEDIA_STOP, "stop"},
           {SDL_SCANCODE_MEDIA_REWIND, "rewind"},
           {SDL_SCANCODE_MEDIA_FAST_FORWARD, "fastForward"},
           {SDL_SCANCODE_WEBOS_CH_UP, "channelUp"},
           {SDL_SCANCODE_WEBOS_CH_DOWN, "channelDown"},
           {SDL_SCANCODE_WEBOS_RED, "red"},
           {SDL_SCANCODE_WEBOS_GREEN, "green"},
           {SDL_SCANCODE_WEBOS_YELLOW, "yellow"},
           {SDL_SCANCODE_WEBOS_BLUE, "blue"},
           {SDL_SCANCODE_0, "0"},
           {SDL_SCANCODE_9, "9"},
           {SDL_SCANCODE_KP_5, "5"},
           {SDL_SCANCODE_BACKSPACE, "delete"},
       }) {
    CAPTURE(eventType);
    CHECK(eventOf(scancode) == eventType);
  }
}

TEST_CASE("remoteKeyFor marks Back, which goes through BackHandler and is never held") {
  for (const auto scancode : {SDL_SCANCODE_WEBOS_BACK, SDL_SCANCODE_AC_BACK, SDL_SCANCODE_ESCAPE}) {
    const auto key = remoteKeyFor(scancode, 0);
    REQUIRE(key.has_value());
    CHECK(key->isBack);
    CHECK_FALSE(key->hasLongPress());
  }
  CHECK_FALSE(remoteKeyFor(SDL_SCANCODE_RETURN, 0)->isBack);
}

TEST_CASE("remoteKeyFor gives a hold event to the keys react-native-tvos reports held, and to no other") {
  const auto select = remoteKeyFor(SDL_SCANCODE_RETURN, 0);
  const auto up = remoteKeyFor(SDL_SCANCODE_UP, 0);
  const auto playPause = remoteKeyFor(SDL_SCANCODE_MEDIA_PLAY_PAUSE, 0);
  const auto play = remoteKeyFor(SDL_SCANCODE_MEDIA_PLAY, 0);

  CHECK(select->longEventType == "longSelect");
  CHECK(up->longEventType == "longUp");
  CHECK(playPause->longEventType == "longPlayPause");
  CHECK_FALSE(play->hasLongPress());
  CHECK_FALSE(remoteKeyFor(SDL_SCANCODE_WEBOS_RED, 0)->hasLongPress());
}

TEST_CASE("remoteKeyFor matches a key SDL has no scancode for on the key code webOS sent") {
  CHECK(eventOf(SDL_SCANCODE_UNKNOWN, 352) == "select");
  CHECK(eventOf(SDL_SCANCODE_UNKNOWN, 353) == "select");
  CHECK(eventOf(SDL_SCANCODE_UNKNOWN, 358) == "info");
  CHECK(eventOf(SDL_SCANCODE_UNKNOWN, 0) == "none");
  CHECK(eventOf(SDL_SCANCODE_UNKNOWN, 999) == "none");
  CHECK(eventOf(SDL_SCANCODE_UP, 352) == "up");
}

TEST_CASE("remoteKeyFor answers nothing for a key apps do not see") {
  CHECK(eventOf(SDL_SCANCODE_A) == "none");
  CHECK(eventOf(SDL_SCANCODE_TAB) == "none");
  CHECK(eventOf(SDL_SCANCODE_F13) == "none");
}

TEST_CASE("scancodeForKeyName finds the key a simulator key name presses, whatever its case") {
  CHECK(scancodeForKeyName("Back") == SDL_SCANCODE_WEBOS_BACK);
  CHECK(scancodeForKeyName("back") == SDL_SCANCODE_WEBOS_BACK);
  CHECK(scancodeForKeyName("PLAYPAUSE") == SDL_SCANCODE_MEDIA_PLAY_PAUSE);
  CHECK(scancodeForKeyName("Enter") == scancodeForKeyName("Select"));
  CHECK(scancodeForKeyName("Ok") == SDL_SCANCODE_RETURN);
  CHECK(scancodeForKeyName("7") == SDL_SCANCODE_7);
  CHECK(scancodeForKeyName("NoSuchKey") == SDL_SCANCODE_UNKNOWN);
  CHECK(scancodeForKeyName("") == SDL_SCANCODE_UNKNOWN);
}

TEST_CASE("every remote key the simulator can name reaches the app as its event") {
  for (const auto &[name, eventType] : std::vector<std::pair<std::string, std::string>>{
           {"Up", "up"},
           {"Down", "down"},
           {"Left", "left"},
           {"Right", "right"},
           {"Select", "select"},
           {"Back", "back"},
           {"Play", "play"},
           {"Pause", "pause"},
           {"PlayPause", "playPause"},
           {"Stop", "stop"},
           {"Record", "record"},
           {"Rewind", "rewind"},
           {"FastForward", "fastForward"},
           {"Next", "next"},
           {"Previous", "previous"},
           {"ChannelUp", "channelUp"},
           {"ChannelDown", "channelDown"},
           {"Guide", "guide"},
           {"Info", "info"},
           {"Red", "red"},
           {"Green", "green"},
           {"Yellow", "yellow"},
           {"Blue", "blue"},
           {"0", "0"},
           {"Backspace", "delete"},
           {"Escape", "back"},
       }) {
    CAPTURE(name);
    CHECK(eventOf(scancodeForKeyName(name)) == eventType);
  }
}

} // namespace rnwebos
