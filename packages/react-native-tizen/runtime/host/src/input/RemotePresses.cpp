#include "RemotePresses.h"

#include "../logging/Log.h"

#include <string>
#include <utility>

namespace rntizen {

namespace {

constexpr double kHoldDelaySeconds = 0.3;

// A repeating remote re-sends a held key well within a second, so a key still
// down after a second of silence lost its release: a Samsung long press can
// turn into another key (docs/reference/tizen-input.md, section 7).
constexpr unsigned int kLostReleaseMs = 1000;

} // namespace

RemotePresses::RemotePresses(EventHandler onEvent, KeyFilter keyFilter)
    : onEvent_(std::move(onEvent)), keyFilter_(std::move(keyFilter)) {}

RemotePresses::~RemotePresses() {
  for (const auto &[keycode, press] : presses_) {
    if (press.holdTimer != nullptr) ecore_timer_del(press.holdTimer);
  }
}

bool RemotePresses::keyDown(const Ecore_Event_Key &key) {
  if (key.key == nullptr) return false;
  if (keyFilter_ && keyFilter_(key, true)) return true;
  const auto remoteKey = remoteKeyFor(key.key);
  if (!remoteKey) {
    log(LogLevel::Debug, std::string("Unmapped key: ") + key.key);
    return false;
  }
  track(key, *remoteKey);
  return true;
}

bool RemotePresses::keyUp(const Ecore_Event_Key &key) {
  const bool used = keyFilter_ && keyFilter_(key, false);
  auto down = presses_.extract(key.keycode);
  if (!down.empty()) finish(down.mapped(), !used);
  return used || !down.empty();
}

void RemotePresses::releaseAll() {
  auto presses = std::exchange(presses_, {});
  for (auto &[keycode, press] : presses) finish(press, false);
}

void RemotePresses::track(const Ecore_Event_Key &key, const RemoteKey &remoteKey) {
  if (auto down = presses_.find(key.keycode); down != presses_.end()) {
    if (key.timestamp - down->second.lastSeen < kLostReleaseMs) {
      down->second.lastSeen = key.timestamp;
      return;
    }
    auto lost = presses_.extract(down);
    finish(lost.mapped(), false);
  }
  auto &press = presses_[key.keycode];
  press = Press{.owner = this, .key = remoteKey, .lastSeen = key.timestamp, .holdTimer = nullptr, .isHeld = false};
  if (remoteKey.hasLongPress()) press.holdTimer = ecore_timer_add(kHoldDelaySeconds, onHoldStarted, &press);
}

void RemotePresses::finish(Press &press, bool released) {
  if (press.holdTimer != nullptr) {
    ecore_timer_del(press.holdTimer);
    press.holdTimer = nullptr;
  }
  if (press.isHeld) {
    onEvent_({.eventType = press.key.longEventType, .keyAction = 1, .isBack = false});
  } else if (released) {
    onEvent_({.eventType = press.key.eventType, .keyAction = 1, .isBack = press.key.isBack});
  }
}

Eina_Bool RemotePresses::onHoldStarted(void *data) {
  auto &press = *static_cast<Press *>(data);
  press.holdTimer = nullptr;
  press.isHeld = true;
  press.owner->onEvent_({.eventType = press.key.longEventType, .keyAction = 0, .isBack = false});
  return ECORE_CALLBACK_CANCEL;
}

} // namespace rntizen
