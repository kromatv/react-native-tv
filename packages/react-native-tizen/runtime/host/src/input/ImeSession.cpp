#include "ImeSession.h"

#include "../logging/Log.h"

#include <Ecore_Evas.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace rntizen {

namespace {

bool isNavigationKey(const char *key) {
  if (key == nullptr) return false;
  if (std::strncmp(key, "XF86", 4) == 0) return true;
  static constexpr std::array<std::string_view, 11> kNavigation{
      "Up", "Down", "Left", "Right", "KP_Up", "KP_Down", "KP_Left", "KP_Right", "Return", "KP_Enter", "Select"};
  for (auto name : kNavigation) {
    if (name == key) return true;
  }
  return false;
}

unsigned int imfModifiers(unsigned int modifiers) {
  unsigned int result = ECORE_IMF_KEYBOARD_MODIFIER_NONE;
  if (modifiers & ECORE_EVENT_MODIFIER_SHIFT) result |= ECORE_IMF_KEYBOARD_MODIFIER_SHIFT;
  if (modifiers & ECORE_EVENT_MODIFIER_ALT) result |= ECORE_IMF_KEYBOARD_MODIFIER_ALT;
  if (modifiers & ECORE_EVENT_MODIFIER_CTRL) result |= ECORE_IMF_KEYBOARD_MODIFIER_CTRL;
  if (modifiers & ECORE_EVENT_MODIFIER_WIN) result |= ECORE_IMF_KEYBOARD_MODIFIER_WIN;
  if (modifiers & ECORE_EVENT_MODIFIER_ALTGR) result |= ECORE_IMF_KEYBOARD_MODIFIER_ALTGR;
  return result;
}

unsigned int imfLocks(unsigned int modifiers) {
  unsigned int result = ECORE_IMF_KEYBOARD_LOCK_NONE;
  if (modifiers & ECORE_EVENT_LOCK_NUM) result |= ECORE_IMF_KEYBOARD_LOCK_NUM;
  if (modifiers & ECORE_EVENT_LOCK_CAPS) result |= ECORE_IMF_KEYBOARD_LOCK_CAPS;
  if (modifiers & ECORE_EVENT_LOCK_SCROLL) result |= ECORE_IMF_KEYBOARD_LOCK_SCROLL;
  return result;
}

template <typename ImfKey>
ImfKey imfKey(const Ecore_Event_Key &key) {
  ImfKey event{};
  event.keyname = key.keyname;
  event.modifiers = static_cast<Ecore_IMF_Keyboard_Modifiers>(imfModifiers(key.modifiers));
  event.locks = static_cast<Ecore_IMF_Keyboard_Locks>(imfLocks(key.modifiers));
  event.key = key.key;
  event.string = key.string;
  event.compose = key.compose;
  event.timestamp = key.timestamp;
  event.dev_name = "";
  event.dev_class = ECORE_IMF_DEVICE_CLASS_NONE;
  event.dev_subclass = ECORE_IMF_DEVICE_SUBCLASS_NONE;
  event.keycode = key.keycode;
  return event;
}

Ecore_IMF_Context *createContext(Evas *evas) {
  ecore_imf_init();
  const char *id = ecore_imf_context_default_id_get();
  Eina_List *available = id == nullptr ? ecore_imf_context_available_ids_get() : nullptr;
  if (available != nullptr) id = static_cast<const char *>(eina_list_data_get(available));
  auto *context = id != nullptr ? ecore_imf_context_add(id) : nullptr;
  eina_list_free(available);
  if (context == nullptr) {
    log(LogLevel::Info, "No input method: typing comes from key events only");
    ecore_imf_shutdown();
    return nullptr;
  }
  log(LogLevel::Debug, std::string("Input method context: ") + (id != nullptr ? id : "?"));
  auto *window = reinterpret_cast<void *>(ecore_evas_window_get(ecore_evas_ecore_evas_get(evas)));
  ecore_imf_context_client_window_set(context, window);
  ecore_imf_context_client_canvas_set(context, evas);
  ecore_imf_context_input_panel_language_set(context, ECORE_IMF_INPUT_PANEL_LANG_AUTOMATIC);
  return context;
}

} // namespace

ImeSession::ImeSession(Evas *evas) : context_(createContext(evas)) {
  if (context_ == nullptr) return;
  ecore_imf_context_event_callback_add(context_, ECORE_IMF_CALLBACK_COMMIT, onCommit, this);
  ecore_imf_context_event_callback_add(context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onPreedit, this);
  ecore_imf_context_event_callback_add(context_, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, onDeleteSurrounding, this);
  ecore_imf_context_retrieve_surrounding_callback_set(context_, onSurrounding, this);
  ecore_imf_context_input_panel_event_callback_add(context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT, onPanelState, this);
}

ImeSession::~ImeSession() {
  if (context_ == nullptr) return;
  end();
  ecore_imf_context_input_panel_event_callback_del(context_, ECORE_IMF_INPUT_PANEL_STATE_EVENT, onPanelState);
  ecore_imf_context_event_callback_del(context_, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, onDeleteSurrounding);
  ecore_imf_context_event_callback_del(context_, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onPreedit);
  ecore_imf_context_event_callback_del(context_, ECORE_IMF_CALLBACK_COMMIT, onCommit);
  ecore_imf_context_del(context_);
  ecore_imf_shutdown();
}

bool ImeSession::isAvailable() const {
  return context_ != nullptr;
}

void ImeSession::begin(ImeClient &client, const ImeOptions &options) {
  client_ = &client;
  if (context_ == nullptr) return;
  ecore_imf_context_reset(context_);
  ecore_imf_context_input_panel_layout_set(context_, options.layout);
  ecore_imf_context_input_panel_layout_variation_set(context_, options.layoutVariation);
  ecore_imf_context_input_panel_return_key_type_set(context_, options.returnKey);
  ecore_imf_context_autocapital_type_set(context_, options.autocapital);
  ecore_imf_context_prediction_allow_set(context_, options.prediction ? EINA_TRUE : EINA_FALSE);
  ecore_imf_context_input_mode_set(context_, options.secure ? ECORE_IMF_INPUT_MODE_INVISIBLE : ECORE_IMF_INPUT_MODE_FULL);
  unsigned int hints = options.prediction ? ECORE_IMF_INPUT_HINT_AUTO_COMPLETE : ECORE_IMF_INPUT_HINT_NONE;
  if (options.secure) hints |= ECORE_IMF_INPUT_HINT_SENSITIVE_DATA;
  if (options.multiline) hints |= ECORE_IMF_INPUT_HINT_MULTILINE;
  ecore_imf_context_input_hint_set(context_, static_cast<Ecore_IMF_Input_Hints>(hints));
  ecore_imf_context_input_panel_enabled_set(context_, options.showPanel ? EINA_TRUE : EINA_FALSE);
  ecore_imf_context_focus_in(context_);
  if (options.showPanel) ecore_imf_context_input_panel_show(context_);
}

void ImeSession::end() {
  client_ = nullptr;
  if (context_ == nullptr) return;
  ecore_imf_context_reset(context_);
  ecore_imf_context_focus_out(context_);
  ecore_imf_context_input_panel_hide(context_);
}

bool ImeSession::filter(const Ecore_Event_Key &key, bool isDown) {
  if (context_ == nullptr || client_ == nullptr) return false;
  if (!isPanelShown() && isNavigationKey(key.key)) return false;
  Ecore_IMF_Event event{};
  if (isDown) {
    event.key_down = imfKey<Ecore_IMF_Event_Key_Down>(key);
    return ecore_imf_context_filter_event(context_, ECORE_IMF_EVENT_KEY_DOWN, &event) == EINA_TRUE;
  }
  event.key_up = imfKey<Ecore_IMF_Event_Key_Up>(key);
  return ecore_imf_context_filter_event(context_, ECORE_IMF_EVENT_KEY_UP, &event) == EINA_TRUE;
}

void ImeSession::showPanel() {
  if (context_ != nullptr) ecore_imf_context_input_panel_show(context_);
}

void ImeSession::hidePanel() {
  if (context_ != nullptr) ecore_imf_context_input_panel_hide(context_);
}

bool ImeSession::isPanelShown() const {
  if (context_ == nullptr) return false;
  const auto state = ecore_imf_context_input_panel_state_get(context_);
  return state == ECORE_IMF_INPUT_PANEL_STATE_SHOW || state == ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW;
}

void ImeSession::caretMoved(int position, Evas_Coord x, Evas_Coord y, Evas_Coord width, Evas_Coord height) {
  if (context_ == nullptr || client_ == nullptr) return;
  ecore_imf_context_cursor_position_set(context_, position);
  ecore_imf_context_cursor_location_set(context_, x, y, width, height);
}

void ImeSession::reset() {
  if (context_ != nullptr) ecore_imf_context_reset(context_);
}

void ImeSession::onCommit(void *self, Ecore_IMF_Context *, void *info) {
  auto &session = *static_cast<ImeSession *>(self);
  if (session.client_ != nullptr && info != nullptr) session.client_->imeCommit(static_cast<const char *>(info));
}

void ImeSession::onPreedit(void *self, Ecore_IMF_Context *context, void *) {
  auto &session = *static_cast<ImeSession *>(self);
  if (session.client_ == nullptr) return;
  char *text = nullptr;
  int cursor = 0;
  ecore_imf_context_preedit_string_get(context, &text, &cursor);
  if (text != nullptr && *text != '\0') session.client_->imePreedit(text, cursor);
  std::free(text);
}

void ImeSession::onDeleteSurrounding(void *self, Ecore_IMF_Context *, void *info) {
  auto &session = *static_cast<ImeSession *>(self);
  const auto *event = static_cast<const Ecore_IMF_Event_Delete_Surrounding *>(info);
  if (session.client_ != nullptr && event != nullptr) session.client_->imeDeleteSurrounding(event->offset, event->n_chars);
}

Eina_Bool ImeSession::onSurrounding(void *self, Ecore_IMF_Context *, char **text, int *cursor) {
  auto &session = *static_cast<ImeSession *>(self);
  if (session.client_ == nullptr) return EINA_FALSE;
  int position = 0;
  const auto surrounding = session.client_->imeSurrounding(position);
  if (text != nullptr) *text = strdup(surrounding.c_str());
  if (cursor != nullptr) *cursor = position;
  return EINA_TRUE;
}

void ImeSession::onPanelState(void *self, Ecore_IMF_Context *, int state) {
  auto &session = *static_cast<ImeSession *>(self);
  const auto previous = std::exchange(session.panelState_, static_cast<Ecore_IMF_Input_Panel_State>(state));
  if (session.panelState_ == ECORE_IMF_INPUT_PANEL_STATE_HIDE && previous != ECORE_IMF_INPUT_PANEL_STATE_HIDE &&
      session.client_ != nullptr) {
    session.client_->imePanelHidden();
  }
}

} // namespace rntizen
