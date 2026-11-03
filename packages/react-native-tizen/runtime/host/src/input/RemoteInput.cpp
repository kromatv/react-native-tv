#include "RemoteInput.h"

#include "../logging/Log.h"

#include <Ecore_Evas.h>
#include <efl_extension.h>

#include <string>
#include <utility>

namespace rntizen {

namespace {

Ecore_Window windowIdOf(Evas_Object *window) {
  auto *ecoreEvas = ecore_evas_ecore_evas_get(evas_object_evas_get(window));
  return ecoreEvas != nullptr ? ecore_evas_window_get(ecoreEvas) : 0;
}

} // namespace

RemoteInput::RemoteInput(Evas_Object *window, RemotePresses::EventHandler onEvent, RemotePresses::KeyFilter keyFilter)
    : window_(window),
      windowId_(windowIdOf(window)),
      presses_(std::move(onEvent), std::move(keyFilter)),
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
}

void RemoteInput::releaseAll() {
  presses_.releaseAll();
}

bool RemoteInput::isForWindow(const Ecore_Event_Key &key) const {
  return windowId_ == 0 || key.event_window == windowId_ || key.window == windowId_;
}

Eina_Bool RemoteInput::onKeyDown(void *self, int, void *event) {
  auto &input = *static_cast<RemoteInput *>(self);
  const auto &key = *static_cast<const Ecore_Event_Key *>(event);
  if (!input.isForWindow(key)) return ECORE_CALLBACK_PASS_ON;
  return input.presses_.keyDown(key) ? ECORE_CALLBACK_DONE : ECORE_CALLBACK_PASS_ON;
}

Eina_Bool RemoteInput::onKeyUp(void *self, int, void *event) {
  auto &input = *static_cast<RemoteInput *>(self);
  const auto &key = *static_cast<const Ecore_Event_Key *>(event);
  if (!input.isForWindow(key)) return ECORE_CALLBACK_PASS_ON;
  return input.presses_.keyUp(key) ? ECORE_CALLBACK_DONE : ECORE_CALLBACK_PASS_ON;
}

void RemoteInput::onFocusOut(void *self, Evas_Object *, void *) {
  static_cast<RemoteInput *>(self)->releaseAll();
}

} // namespace rntizen
