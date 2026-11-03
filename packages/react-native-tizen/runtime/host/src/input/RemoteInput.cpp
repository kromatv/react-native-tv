#include "RemoteInput.h"

#include "../logging/Log.h"

#include <Ecore_Evas.h>
#include <efl_extension.h>

#include <string>
#include <utility>

namespace rntizen {

namespace {

constexpr double kHoldDelaySeconds = 0.3;

// A repeating remote re-sends a held key well within a second, so a key still
// down after a second of silence lost its release: a Samsung long press can
// turn into another key (docs/reference/tizen-input.md, section 7).
constexpr unsigned int kLostReleaseMs = 1000;

Ecore_Window windowIdOf(Evas_Object *window) {
  auto *ecoreEvas = ecore_evas_ecore_evas_get(evas_object_evas_get(window));
  return ecoreEvas != nullptr ? ecore_evas_window_get(ecoreEvas) : 0;
}

} // namespace

RemoteInput::RemoteInput(Evas_Object *window, EventHandler onEvent, KeyFilter keyFilter)
    : window_(window),
      windowId_(windowIdOf(window)),
      onEvent_(std::move(onEvent)),
      keyFilter_(std::move(keyFilter)),
      keyDown_(ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, onKeyDown, this)),
      keyUp_(ecore_event_handler_add(ECORE_EVENT_KEY_UP, onKeyUp, this)) {
  for (auto keyName : grabbedKeyNames()) {
    const std::string name(keyName);
    if (!eext_win_keygrab_set(window_, name.c_str())) log(LogLevel::Warning, "Cannot grab the " + name + " key");
  }
  evas_object_smart_callback_add(window_, "focus,out", onFocusOut, this);
}

RemoteInput::~RemoteInput() {
  evas_object_smart_callback_del_full(window_, "focus,out", onFocusOut, this);
  ecore_event_handler_del(keyUp_);
  ecore_event_handler_del(keyDown_);
  for (auto &[keycode, press] : presses_) {
    if (press.holdTimer != nullptr) ecore_timer_del(press.holdTimer);
  }
}

void RemoteInput::releaseAll() {
  auto presses = std::exchange(presses_, {});
  for (auto &[keycode, press] : presses) finish(press, false);
}

bool RemoteInput::isForWindow(const Ecore_Event_Key &key) const {
  return windowId_ == 0 || key.event_window == windowId_ || key.window == windowId_;
}

Eina_Bool RemoteInput::onKeyDown(void *self, int, void *event) {
  auto &input = *static_cast<RemoteInput *>(self);
  const auto &key = *static_cast<const Ecore_Event_Key *>(event);
  if (key.key == nullptr || !input.isForWindow(key)) return ECORE_CALLBACK_PASS_ON;
  if (input.keyFilter_ && input.keyFilter_(key, true)) return ECORE_CALLBACK_DONE;
  auto remoteKey = remoteKeyFor(key.key);
  if (!remoteKey) {
    log(LogLevel::Debug, std::string("Unmapped key: ") + key.key);
    return ECORE_CALLBACK_PASS_ON;
  }
  input.keyDown(key, *remoteKey);
  return ECORE_CALLBACK_DONE;
}

Eina_Bool RemoteInput::onKeyUp(void *self, int, void *event) {
  auto &input = *static_cast<RemoteInput *>(self);
  const auto &key = *static_cast<const Ecore_Event_Key *>(event);
  if (!input.isForWindow(key)) return ECORE_CALLBACK_PASS_ON;
  if (input.keyFilter_ && input.keyFilter_(key, false)) return ECORE_CALLBACK_DONE;
  return input.keyUp(key) ? ECORE_CALLBACK_DONE : ECORE_CALLBACK_PASS_ON;
}

void RemoteInput::keyDown(const Ecore_Event_Key &key, const RemoteKey &remoteKey) {
  if (auto down = presses_.find(key.keycode); down != presses_.end()) {
    if (key.timestamp - down->second.lastSeen < kLostReleaseMs) {
      down->second.lastSeen = key.timestamp;
      return;
    }
    auto lost = down->second;
    presses_.erase(down);
    finish(lost, false);
  }
  auto &press = presses_[key.keycode];
  press = Press{.owner = this, .key = remoteKey, .lastSeen = key.timestamp, .holdTimer = nullptr, .isHeld = false};
  if (remoteKey.hasLongPress()) press.holdTimer = ecore_timer_add(kHoldDelaySeconds, onHoldStarted, &press);
}

bool RemoteInput::keyUp(const Ecore_Event_Key &key) {
  auto down = presses_.find(key.keycode);
  if (down == presses_.end()) return false;
  auto press = down->second;
  presses_.erase(down);
  finish(press, true);
  return true;
}

void RemoteInput::finish(Press &press, bool released) {
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

Eina_Bool RemoteInput::onHoldStarted(void *data) {
  auto &press = *static_cast<Press *>(data);
  press.holdTimer = nullptr;
  press.isHeld = true;
  press.owner->onEvent_({.eventType = press.key.longEventType, .keyAction = 0, .isBack = false});
  return ECORE_CALLBACK_CANCEL;
}

void RemoteInput::onFocusOut(void *self, Evas_Object *, void *) {
  static_cast<RemoteInput *>(self)->releaseAll();
}

} // namespace rntizen
