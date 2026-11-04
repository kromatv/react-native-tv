#include "RemoteKeyMap.h"

#include <array>
#include <utility>

namespace rntizen {

namespace {

consteval RemoteKey press(std::string_view eventType) {
  return {.eventType = eventType, .longEventType = {}, .isBack = false};
}

consteval RemoteKey hold(std::string_view eventType, std::string_view longEventType) {
  return {.eventType = eventType, .longEventType = longEventType, .isBack = false};
}

constexpr RemoteKey kBack{.eventType = "back", .longEventType = {}, .isBack = true};

constexpr std::array<std::pair<std::string_view, RemoteKey>, 44> kKeys{{
    {"Up", hold("up", "longUp")},
    {"Down", hold("down", "longDown")},
    {"Left", hold("left", "longLeft")},
    {"Right", hold("right", "longRight")},
    {"Return", hold("select", "longSelect")},
    {"KP_Enter", hold("select", "longSelect")},
    {"XF86Back", kBack},
    {"Escape", kBack},
    {"XF86PlayBack", hold("playPause", "longPlayPause")},
    {"XF86AudioPlayPause", hold("playPause", "longPlayPause")},
    {"XF86AudioPlay", press("play")},
    {"XF86AudioPause", press("pause")},
    {"XF86AudioStop", press("stop")},
    {"XF86AudioRecord", press("record")},
    {"XF86AudioRewind", press("rewind")},
    {"XF86AudioNext", press("fastForward")},
    {"XF86AudioForward", press("fastForward")},
    {"XF86NextChapter", press("next")},
    {"XF86PreviousChapter", press("previous")},
    {"XF86AudioPrev", press("previous")},
    {"XF86RaiseChannel", press("channelUp")},
    {"XF86LowerChannel", press("channelDown")},
    {"XF86ChannelGuide", press("guide")},
    {"XF86Info", press("info")},
    {"XF86SimpleMenu", press("menu")},
    {"XF86Menu", press("menu")},
    {"XF86Caption", press("captions")},
    {"XF86Subtitle", press("captions")},
    {"XF86Red", press("red")},
    {"XF86Green", press("green")},
    {"XF86Yellow", press("yellow")},
    {"XF86Blue", press("blue")},
    {"0", press("0")},
    {"1", press("1")},
    {"2", press("2")},
    {"3", press("3")},
    {"4", press("4")},
    {"5", press("5")},
    {"6", press("6")},
    {"7", press("7")},
    {"8", press("8")},
    {"9", press("9")},
    {"BackSpace", press("delete")},
    {"XF86BTVoice", press("voice")},
}};

constexpr std::array<std::string_view, 23> kGrabbed{
    "XF86Back",         "XF86PlayBack",     "XF86AudioPlayPause", "XF86AudioPlay",     "XF86AudioPause",
    "XF86AudioStop",    "XF86AudioRecord",  "XF86AudioRewind",    "XF86AudioNext",     "XF86AudioForward",
    "XF86AudioPrev",    "XF86NextChapter",  "XF86PreviousChapter", "XF86RaiseChannel", "XF86LowerChannel",
    "XF86ChannelGuide", "XF86Info",         "XF86SimpleMenu",     "XF86Caption",       "XF86Red",
    "XF86Green",        "XF86Yellow",       "XF86Blue"};

} // namespace

std::optional<RemoteKey> remoteKeyFor(std::string_view keySymbol) {
  for (const auto &[symbol, key] : kKeys) {
    if (symbol == keySymbol) return key;
  }
  return std::nullopt;
}

std::span<const std::string_view> grabbedKeyNames() {
  return kGrabbed;
}

} // namespace rntizen
