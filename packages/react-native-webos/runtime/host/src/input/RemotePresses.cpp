#include "RemotePresses.h"

#include "../logging/Log.h"

#include <chrono>
#include <format>
#include <utility>

namespace rnwebos {

namespace {

constexpr auto kHoldDelay = std::chrono::milliseconds(300);

// A repeating remote re-sends a held key well within a second, so a key still
// down after a second of silence lost its release.
constexpr std::uint64_t kLostReleaseMs = 1000;

constexpr std::uint32_t kRawKeyBase = 0x10000;

// A key SDL has no scancode for is told apart by its Linux key code.
std::uint32_t codeOf(const SDL_KeyboardEvent &key) {
  return key.scancode != SDL_SCANCODE_UNKNOWN ? static_cast<std::uint32_t>(key.scancode) : kRawKeyBase + key.raw;
}

std::uint64_t milliseconds(const SDL_KeyboardEvent &key) {
  return key.timestamp / 1'000'000;
}

} // namespace

RemotePresses::RemotePresses(EventHandler onEvent, KeyFilter keyFilter)
    : onEvent_(std::move(onEvent)), keyFilter_(std::move(keyFilter)) {}

RemotePresses::~RemotePresses() {
  for (const auto &[code, press] : presses_) {
    if (press.holdTimer != 0) MainLoop::get().removeTimer(press.holdTimer);
  }
}

bool RemotePresses::keyDown(const SDL_KeyboardEvent &key) {
  if (keyFilter_ && keyFilter_(key, true)) return true;
  const auto remoteKey = remoteKeyFor(key.scancode, key.raw);
  if (!remoteKey) {
    log(LogLevel::Debug, std::format("Unmapped key: scancode {}, code {}", static_cast<int>(key.scancode), key.raw));
    return false;
  }
  track(codeOf(key), key, *remoteKey);
  return true;
}

bool RemotePresses::keyUp(const SDL_KeyboardEvent &key) {
  const bool used = keyFilter_ && keyFilter_(key, false);
  auto down = presses_.extract(codeOf(key));
  if (!down.empty()) finish(down.mapped(), !used);
  return used || !down.empty();
}

void RemotePresses::releaseAll() {
  auto presses = std::exchange(presses_, {});
  for (auto &[code, press] : presses) finish(press, false);
}

void RemotePresses::track(std::uint32_t code, const SDL_KeyboardEvent &key, const RemoteKey &remoteKey) {
  const auto now = milliseconds(key);
  if (auto down = presses_.find(code); down != presses_.end()) {
    if (now - down->second.lastSeenMs < kLostReleaseMs) {
      down->second.lastSeenMs = now;
      return;
    }
    auto lost = presses_.extract(down);
    finish(lost.mapped(), false);
  }
  auto &press = presses_[code];
  press = Press{.key = remoteKey, .lastSeenMs = now, .holdTimer = 0, .isHeld = false};
  if (remoteKey.hasLongPress()) {
    press.holdTimer = MainLoop::get().addTimer(kHoldDelay, [this, code] { holdStarted(code); });
  }
}

void RemotePresses::finish(Press &press, bool released) const {
  if (press.holdTimer != 0) {
    MainLoop::get().removeTimer(press.holdTimer);
    press.holdTimer = 0;
  }
  if (press.isHeld) {
    onEvent_({.eventType = press.key.longEventType, .keyAction = 1, .isBack = false});
  } else if (released) {
    onEvent_({.eventType = press.key.eventType, .keyAction = 1, .isBack = press.key.isBack});
  }
}

void RemotePresses::holdStarted(std::uint32_t code) {
  const auto found = presses_.find(code);
  if (found == presses_.end()) return;
  found->second.holdTimer = 0;
  found->second.isHeld = true;
  onEvent_({.eventType = found->second.key.longEventType, .keyAction = 0, .isBack = false});
}

} // namespace rnwebos
