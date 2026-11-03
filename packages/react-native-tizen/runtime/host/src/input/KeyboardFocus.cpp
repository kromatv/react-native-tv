#include "KeyboardFocus.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace rntizen {

namespace {

std::vector<KeyboardFocus *> &keyboards() {
  static std::vector<KeyboardFocus *> registered;
  return registered;
}

} // namespace

KeyboardFocus::KeyboardFocus(Evas *evas) : evas_(evas) {
  keyboards().push_back(this);
}

KeyboardFocus::~KeyboardFocus() {
  auto &all = keyboards();
  all.erase(std::remove(all.begin(), all.end(), this), all.end());
  if (auto *client = std::exchange(focused_, nullptr)) client->keyboardLost();
  ime_.reset();
}

KeyboardFocus *KeyboardFocus::of(Evas *evas) {
  const auto &all = keyboards();
  const auto found = std::find_if(all.begin(), all.end(), [evas](auto *keyboard) { return keyboard->evas_ == evas; });
  return found == all.end() ? nullptr : *found;
}

ImeSession &KeyboardFocus::ime() {
  if (!ime_) ime_ = std::make_unique<ImeSession>(evas_);
  return *ime_;
}

void KeyboardFocus::focus(KeyboardClient &client, const ImeOptions &options) {
  if (focused_ == &client) return;
  if (auto *previous = std::exchange(focused_, &client)) {
    ime().end();
    previous->keyboardLost();
  }
  ime().begin(client, options);
}

void KeyboardFocus::blur(KeyboardClient &client) {
  if (focused_ != &client) return;
  focused_ = nullptr;
  ime().end();
}

bool KeyboardFocus::hasFocus(const KeyboardClient &client) const {
  return focused_ == &client;
}

bool KeyboardFocus::handleKey(const Ecore_Event_Key &key, bool isDown) {
  const auto taken = takenKeys_.find(key.keycode);
  const bool held = taken != takenKeys_.end();
  if (!isDown) {
    if (!held) return false;
    const auto taker = taken->second;
    takenKeys_.erase(taken);
    if (taker == Taker::InputMethod) {
      if (ime_) ime_->filter(key, false);
    } else if (focused_ != nullptr) {
      focused_->keyUp(key);
    }
    return true;
  }
  if (focused_ == nullptr) return held;
  const auto taker = held ? taken->second : Taker::InputMethod;
  if (taker == Taker::InputMethod && ime().filter(key, true)) {
    takenKeys_[key.keycode] = Taker::InputMethod;
    return true;
  }
  if (focused_ == nullptr) return held;
  const bool used = focused_->keyDown(key, held && taker == Taker::Field);
  if (used) takenKeys_[key.keycode] = Taker::Field;
  return used || held;
}

} // namespace rntizen
