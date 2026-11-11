#include "RemoteKeyMap.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace rnwebos {

namespace {

consteval RemoteKey press(std::string_view eventType) {
  return {.eventType = eventType, .longEventType = {}, .isBack = false};
}

consteval RemoteKey hold(std::string_view eventType, std::string_view longEventType) {
  return {.eventType = eventType, .longEventType = longEventType, .isBack = false};
}

constexpr RemoteKey kBack{.eventType = "back", .longEventType = {}, .isBack = true};
constexpr RemoteKey kSelect = hold("select", "longSelect");

// The Linux key codes of the remote keys LG's SDL has no scancode for.
constexpr Uint16 kRawOk = 352;
constexpr Uint16 kRawSelect = 353;
constexpr Uint16 kRawInfo = 358;

constexpr std::array<std::pair<SDL_Scancode, RemoteKey>, 56> kKeys{{
    {SDL_SCANCODE_UP, hold("up", "longUp")},
    {SDL_SCANCODE_DOWN, hold("down", "longDown")},
    {SDL_SCANCODE_LEFT, hold("left", "longLeft")},
    {SDL_SCANCODE_RIGHT, hold("right", "longRight")},
    {SDL_SCANCODE_RETURN, kSelect},
    {SDL_SCANCODE_KP_ENTER, kSelect},
    {SDL_SCANCODE_WEBOS_BACK, kBack},
    {SDL_SCANCODE_AC_BACK, kBack},
    {SDL_SCANCODE_ESCAPE, kBack},
    {SDL_SCANCODE_MEDIA_PLAY_PAUSE, hold("playPause", "longPlayPause")},
    {SDL_SCANCODE_MEDIA_PLAY, press("play")},
    {SDL_SCANCODE_MEDIA_PAUSE, press("pause")},
    {SDL_SCANCODE_PAUSE, press("pause")},
    {SDL_SCANCODE_MEDIA_STOP, press("stop")},
    {SDL_SCANCODE_MEDIA_RECORD, press("record")},
    {SDL_SCANCODE_MEDIA_REWIND, press("rewind")},
    {SDL_SCANCODE_MEDIA_FAST_FORWARD, press("fastForward")},
    {SDL_SCANCODE_MEDIA_NEXT_TRACK, press("next")},
    {SDL_SCANCODE_MEDIA_PREVIOUS_TRACK, press("previous")},
    {SDL_SCANCODE_WEBOS_CH_UP, press("channelUp")},
    {SDL_SCANCODE_WEBOS_CH_DOWN, press("channelDown")},
    {SDL_SCANCODE_CHANNEL_INCREMENT, press("channelUp")},
    {SDL_SCANCODE_CHANNEL_DECREMENT, press("channelDown")},
    {SDL_SCANCODE_WEBOS_GUIDE, press("guide")},
    {SDL_SCANCODE_WEBOS_RED, press("red")},
    {SDL_SCANCODE_WEBOS_GREEN, press("green")},
    {SDL_SCANCODE_WEBOS_YELLOW, press("yellow")},
    {SDL_SCANCODE_WEBOS_BLUE, press("blue")},
    {SDL_SCANCODE_0, press("0")},
    {SDL_SCANCODE_1, press("1")},
    {SDL_SCANCODE_2, press("2")},
    {SDL_SCANCODE_3, press("3")},
    {SDL_SCANCODE_4, press("4")},
    {SDL_SCANCODE_5, press("5")},
    {SDL_SCANCODE_6, press("6")},
    {SDL_SCANCODE_7, press("7")},
    {SDL_SCANCODE_8, press("8")},
    {SDL_SCANCODE_9, press("9")},
    {SDL_SCANCODE_KP_0, press("0")},
    {SDL_SCANCODE_KP_1, press("1")},
    {SDL_SCANCODE_KP_2, press("2")},
    {SDL_SCANCODE_KP_3, press("3")},
    {SDL_SCANCODE_KP_4, press("4")},
    {SDL_SCANCODE_KP_5, press("5")},
    {SDL_SCANCODE_KP_6, press("6")},
    {SDL_SCANCODE_KP_7, press("7")},
    {SDL_SCANCODE_KP_8, press("8")},
    {SDL_SCANCODE_KP_9, press("9")},
    {SDL_SCANCODE_BACKSPACE, press("delete")},
    {SDL_SCANCODE_F1, press("red")},
    {SDL_SCANCODE_F2, press("green")},
    {SDL_SCANCODE_F3, press("yellow")},
    {SDL_SCANCODE_F4, press("blue")},
    {SDL_SCANCODE_F5, press("info")},
    {SDL_SCANCODE_F6, press("guide")},
    {SDL_SCANCODE_F7, hold("playPause", "longPlayPause")},
}};

constexpr std::array<std::pair<std::string_view, SDL_Scancode>, 38> kKeyNames{{
    {"Up", SDL_SCANCODE_UP},
    {"Down", SDL_SCANCODE_DOWN},
    {"Left", SDL_SCANCODE_LEFT},
    {"Right", SDL_SCANCODE_RIGHT},
    {"Enter", SDL_SCANCODE_RETURN},
    {"Ok", SDL_SCANCODE_RETURN},
    {"Select", SDL_SCANCODE_RETURN},
    {"Back", SDL_SCANCODE_WEBOS_BACK},
    {"Play", SDL_SCANCODE_MEDIA_PLAY},
    {"Pause", SDL_SCANCODE_MEDIA_PAUSE},
    {"PlayPause", SDL_SCANCODE_MEDIA_PLAY_PAUSE},
    {"Stop", SDL_SCANCODE_MEDIA_STOP},
    {"Record", SDL_SCANCODE_MEDIA_RECORD},
    {"Rewind", SDL_SCANCODE_MEDIA_REWIND},
    {"FastForward", SDL_SCANCODE_MEDIA_FAST_FORWARD},
    {"Next", SDL_SCANCODE_MEDIA_NEXT_TRACK},
    {"Previous", SDL_SCANCODE_MEDIA_PREVIOUS_TRACK},
    {"ChannelUp", SDL_SCANCODE_WEBOS_CH_UP},
    {"ChannelDown", SDL_SCANCODE_WEBOS_CH_DOWN},
    {"Guide", SDL_SCANCODE_WEBOS_GUIDE},
    {"Info", SDL_SCANCODE_F5},
    {"Red", SDL_SCANCODE_WEBOS_RED},
    {"Green", SDL_SCANCODE_WEBOS_GREEN},
    {"Yellow", SDL_SCANCODE_WEBOS_YELLOW},
    {"Blue", SDL_SCANCODE_WEBOS_BLUE},
    {"0", SDL_SCANCODE_0},
    {"1", SDL_SCANCODE_1},
    {"2", SDL_SCANCODE_2},
    {"3", SDL_SCANCODE_3},
    {"4", SDL_SCANCODE_4},
    {"5", SDL_SCANCODE_5},
    {"6", SDL_SCANCODE_6},
    {"7", SDL_SCANCODE_7},
    {"8", SDL_SCANCODE_8},
    {"9", SDL_SCANCODE_9},
    {"Backspace", SDL_SCANCODE_BACKSPACE},
    {"Escape", SDL_SCANCODE_ESCAPE},
    {"Tab", SDL_SCANCODE_TAB},
}};

} // namespace

std::optional<RemoteKey> remoteKeyFor(SDL_Scancode scancode, Uint16 raw) {
  if (scancode == SDL_SCANCODE_UNKNOWN) {
    if (raw == kRawOk || raw == kRawSelect) return kSelect;
    if (raw == kRawInfo) return press("info");
    return std::nullopt;
  }
  for (const auto &[code, key] : kKeys) {
    if (code == scancode) return key;
  }
  return std::nullopt;
}

SDL_Scancode scancodeForKeyName(std::string_view name) {
  for (const auto &[keyName, scancode] : kKeyNames) {
    if (std::ranges::equal(keyName, name, [](char a, char b) { return std::tolower(a) == std::tolower(b); })) {
      return scancode;
    }
  }
  return SDL_SCANCODE_UNKNOWN;
}

} // namespace rnwebos
