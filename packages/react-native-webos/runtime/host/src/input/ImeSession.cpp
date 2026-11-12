#include "ImeSession.h"

#include "../logging/Log.h"
#include "../platform/MainLoop.h"

#include <SDL3/SDL_properties.h>

#include <format>
#include <utility>

namespace rnwebos {

ImeSession::ImeSession(SDL_Window *window) : window_(window) {}

ImeSession::~ImeSession() {
  end();
}

bool ImeSession::isAvailable() const {
  return panelSeen_ || (window_ != nullptr && SDL_HasScreenKeyboardSupport());
}

void ImeSession::begin(ImeClient &client, const ImeOptions &options) {
  stop();
  client_ = &client;
  options_ = options;
  for (const auto &sentence : options_.unsupported) {
    if (reported_.insert(sentence).second) log(LogLevel::Warning, "TextInput: " + sentence);
  }
  start();
}

void ImeSession::end() {
  client_ = nullptr;
  stop();
}

void ImeSession::start() {
  emptiedComposition_.reset();
  active_ = true;
  if (window_ == nullptr) return;
  const auto properties = SDL_CreateProperties();
  SDL_SetNumberProperty(properties, SDL_PROP_TEXTINPUT_TYPE_NUMBER, options_.type);
  SDL_SetNumberProperty(properties, SDL_PROP_TEXTINPUT_CAPITALIZATION_NUMBER, options_.capitalization);
  SDL_SetBooleanProperty(properties, SDL_PROP_TEXTINPUT_AUTOCORRECT_BOOLEAN, options_.autocorrect);
  SDL_SetBooleanProperty(properties, SDL_PROP_TEXTINPUT_MULTILINE_BOOLEAN, options_.multiline);
  if (!SDL_StartTextInputWithProperties(window_, properties)) {
    log(LogLevel::Warning, std::format("Cannot start text input: {}", SDL_GetError()));
  }
  SDL_DestroyProperties(properties);
  // SDL's webOS backend lowers the keyboard on a stop without saying so, and
  // then sends no shown event for the next raise: its own flag is all that tells.
  panelShown_ = SDL_ScreenKeyboardShown(window_);
}

void ImeSession::stop() {
  emptiedComposition_.reset();
  panelShown_ = false;
  if (!std::exchange(active_, false) || window_ == nullptr) return;
  SDL_StopTextInput(window_);
}

void ImeSession::showPanel() {
  if (client_ == nullptr) return;
  // webOS raises the keyboard when a text model is activated and at no other
  // time, so showing it again takes a new start of text input.
  stop();
  start();
}

void ImeSession::hidePanel() {
  const bool wasShown = panelShown_;
  stop();
  if (wasShown && client_ != nullptr) client_->imePanelHidden();
}

bool ImeSession::isPanelShown() const {
  return panelShown_;
}

bool ImeSession::isActive() const {
  return active_;
}

void ImeSession::caretMoved(const PixelRect &line, int caretX) {
  if (window_ == nullptr || !active_) return;
  const SDL_Rect area{.x = line.x, .y = line.y, .w = line.width, .h = line.height};
  SDL_SetTextInputArea(window_, &area, caretX - line.x);
}

void ImeSession::reset() {
  emptiedComposition_.reset();
  if (window_ != nullptr && active_) SDL_ClearComposition(window_);
}

bool ImeSession::handleEvent(const SDL_Event &event) {
  switch (event.type) {
    case SDL_EVENT_TEXT_INPUT:
      emptiedComposition_.reset();
      if (client_ != nullptr && active_ && event.text.text != nullptr) client_->imeCommit(event.text.text);
      return true;
    case SDL_EVENT_TEXT_EDITING:
      if (client_ == nullptr || !active_ || event.edit.text == nullptr) return true;
      if (*event.edit.text == '\0') {
        compositionEmptied();
      } else {
        emptiedComposition_.reset();
        client_->imePreedit(event.edit.text, event.edit.start);
      }
      return true;
    case SDL_EVENT_SCREEN_KEYBOARD_SHOWN:
      panelSeen_ = true;
      panelShown_ = active_;
      return true;
    case SDL_EVENT_SCREEN_KEYBOARD_HIDDEN:
      if (std::exchange(panelShown_, false) && client_ != nullptr) client_->imePanelHidden();
      return true;
    default:
      return false;
  }
}

void ImeSession::compositionEmptied() {
  // SDL empties the composition right before it commits it, as two events: the
  // field must see one edit, so the empty one waits for the events queued with it.
  emptiedComposition_ = std::make_shared<bool>(true);
  MainLoop::get().post([this, emptied = std::weak_ptr(emptiedComposition_)] {
    if (const auto pending = emptied.lock(); !pending || pending != emptiedComposition_) return;
    emptiedComposition_.reset();
    if (client_ != nullptr) client_->imePreedit({}, 0);
  });
}

} // namespace rnwebos
