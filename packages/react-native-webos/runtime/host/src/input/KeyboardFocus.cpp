#include "KeyboardFocus.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace rnwebos {

namespace {

constexpr std::uint32_t kRawKeyBase = 0x10000;

std::vector<KeyboardFocus *> &keyboards() {
  static std::vector<KeyboardFocus *> registered;
  return registered;
}

// A key SDL has no scancode for is told apart by its Linux key code.
std::uint32_t codeOf(const SDL_KeyboardEvent &key) {
  return key.scancode != SDL_SCANCODE_UNKNOWN ? static_cast<std::uint32_t>(key.scancode) : kRawKeyBase + key.raw;
}

} // namespace

KeyboardFocus::KeyboardFocus(Scene &scene) : scene_(scene) {
  keyboards().push_back(this);
}

KeyboardFocus::~KeyboardFocus() {
  std::erase(keyboards(), this);
  if (auto *client = std::exchange(focused_, nullptr)) client->keyboardLost();
  ime_.reset();
}

KeyboardFocus *KeyboardFocus::of(const Scene &scene) {
  const auto &all = keyboards();
  const auto found = std::ranges::find_if(all, [&scene](const auto *keyboard) { return &keyboard->scene_ == &scene; });
  return found == all.end() ? nullptr : *found;
}

ImeSession &KeyboardFocus::ime() {
  if (!ime_) {
    const auto *window = scene_.window();
    ime_ = std::make_unique<ImeSession>(window != nullptr ? window->sdlWindow() : nullptr);
  }
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

bool KeyboardFocus::handleKey(const SDL_KeyboardEvent &key, bool isDown) {
  const auto code = codeOf(key);
  const bool held = takenKeys_.contains(code);
  if (!isDown) {
    if (!held) return false;
    takenKeys_.erase(code);
    if (focused_ != nullptr) focused_->keyUp(key);
    return true;
  }
  if (focused_ == nullptr) return held;
  const bool used = focused_->keyDown(key, held);
  if (used) takenKeys_.insert(code);
  return used || held;
}

bool KeyboardFocus::handleTextEvent(const SDL_Event &event) {
  return ime().handleEvent(event);
}

void KeyboardFocus::releaseKeys() {
  takenKeys_.clear();
}

} // namespace rnwebos
