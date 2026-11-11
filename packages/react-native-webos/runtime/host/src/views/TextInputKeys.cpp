#include "TextInputView.h"

#include "../input/RemoteKeyMap.h"

#include <algorithm>
#include <string_view>

namespace rnwebos {

namespace {

constexpr std::size_t kKeyPressUnits = 2;
constexpr SDL_Keycode kFirstPrintable = SDLK_SPACE;

bool isBack(const SDL_KeyboardEvent &key) {
  const auto remoteKey = remoteKeyFor(key.scancode, key.raw);
  return remoteKey && remoteKey->isBack;
}

bool isOk(const SDL_KeyboardEvent &key) {
  const auto remoteKey = remoteKeyFor(key.scancode, key.raw);
  return remoteKey && remoteKey->eventType == "select";
}

bool hasShift(const SDL_KeyboardEvent &key) {
  return (key.mod & SDL_KMOD_SHIFT) != 0;
}

// SDL sends what such a key types as a text input event of its own.
bool typesText(const SDL_KeyboardEvent &key) {
  constexpr SDL_Keymod kCommandModifiers = SDL_KMOD_CTRL | SDL_KMOD_ALT | SDL_KMOD_GUI;
  if ((key.mod & kCommandModifiers) != 0) return false;
  return key.key >= kFirstPrintable && key.key != SDLK_DELETE && (key.key & (SDLK_SCANCODE_MASK | SDLK_EXTENDED_MASK)) == 0;
}

std::optional<std::string> keyPressFor(std::u32string_view text) {
  if (text == U"\n") return "Enter";
  if (text.empty() || utf16Length(text) > kKeyPressUnits) return std::nullopt;
  return encodeUtf8(text);
}

} // namespace

bool TextInputView::keyDown(const SDL_KeyboardEvent &key, bool repeat) {
  const bool extend = hasShift(key);
  if (isBack(key)) return true;
  if (isOk(key)) {
    if (submitBehavior() == facebook::react::SubmitBehavior::Newline && !reopensKeyboard()) insert(U"\n");
    return true;
  }
  switch (key.scancode) {
    case SDL_SCANCODE_BACKSPACE:
      if (buffer_.deleteBackward()) {
        edited("Backspace");
      } else {
        emit("keyPress", folly::dynamic::object("key", "Backspace"));
      }
      return true;
    case SDL_SCANCODE_DELETE:
      if (buffer_.deleteForward()) edited(std::nullopt);
      return true;
    case SDL_SCANCODE_CLEAR:
      if (!repeat && buffer_.clear()) edited("Backspace");
      return true;
    case SDL_SCANCODE_LEFT:
      return stepCaret(-1, extend);
    case SDL_SCANCODE_RIGHT:
      return stepCaret(1, extend);
    case SDL_SCANCODE_UP:
      return stepLine(-1, extend);
    case SDL_SCANCODE_DOWN:
      return stepLine(1, extend);
    case SDL_SCANCODE_HOME:
      moveCaret(0, extend);
      return true;
    case SDL_SCANCODE_END:
      moveCaret(buffer_.size(), extend);
      return true;
    default:
      break;
  }
  auto *focus = keyboard();
  if (!typesText(key) || focus == nullptr || !focus->ime().isActive()) return false;
  typedWithKeys_ = true;
  return true;
}

void TextInputView::keyUp(const SDL_KeyboardEvent &key) {
  if (isOk(key)) {
    okReleased();
  } else if (isBack(key)) {
    backReleased();
  }
}

void TextInputView::insert(std::u32string_view text) {
  const auto key = keyPressFor(text);
  if (buffer_.insert(text, maxLength())) {
    edited(key);
  } else if (key) {
    emit("keyPress", folly::dynamic::object("key", *key));
  }
}

void TextInputView::moveCaret(std::size_t position, bool extend) {
  buffer_.moveTo(position, extend);
  emitSelection();
  refresh();
}

bool TextInputView::stepCaret(int delta, bool extend) {
  if (!extend && buffer_.hasSelection()) {
    const auto selection = buffer_.selection();
    moveCaret(delta < 0 ? selection.start : selection.end, false);
    return true;
  }
  const auto caret = buffer_.caret();
  if ((delta < 0 && caret == 0) || (delta > 0 && caret >= buffer_.size())) return leave();
  moveCaret(delta < 0 ? caret - 1 : caret + 1, extend);
  return true;
}

bool TextInputView::stepLine(int lines, bool extend) {
  const auto landed = buffer_.size() > 0 ? layer_.caretOnLine(lines) : std::nullopt;
  if (!landed) return leave();
  moveCaret(inputProps().secureTextEntry ? *landed : buffer_.positionAtUtf16(*landed), extend);
  return true;
}

bool TextInputView::leave() {
  blur();
  return false;
}

// A key that types lowers the TV's keyboard by itself, so OK brings the
// keyboard back only when the user closed it: with Back, or on the keyboard.
bool TextInputView::reopensKeyboard() const {
  auto *focus = keyboard();
  return keyboardClosed_ && !typedWithKeys_ && focus != nullptr && focus->ime().isAvailable() &&
      !focus->ime().isPanelShown();
}

void TextInputView::okReleased() {
  if (reopensKeyboard()) {
    keyboardClosed_ = false;
    keyboard()->ime().showPanel();
    return;
  }
  if (submitBehavior() != facebook::react::SubmitBehavior::Newline) submit();
}

void TextInputView::backReleased() {
  auto *focus = keyboard();
  if (focus != nullptr && focus->ime().isPanelShown()) {
    focus->ime().hidePanel();
    return;
  }
  blur();
}

void TextInputView::imeCommit(std::string_view text) {
  const auto decoded = decodeUtf8(text);
  const auto key = keyPressFor(decoded);
  if (buffer_.commit(decoded, maxLength())) {
    edited(key);
  } else if (key) {
    emit("keyPress", folly::dynamic::object("key", *key));
  }
}

void TextInputView::imePreedit(std::string_view text, int cursor) {
  const auto decoded = decodeUtf8(text);
  const auto before = buffer_.composing();
  if (decoded.empty() && !before) return;
  const auto composedBefore = before ? before->end - before->start : 0;
  const auto caret = cursor < 0 ? decoded.size() : static_cast<std::size_t>(cursor);
  if (!buffer_.compose(decoded, caret, maxLength())) {
    refresh();
    return;
  }
  const auto after = buffer_.composing();
  const auto composedAfter = after ? after->end - after->start : 0;
  std::optional<std::string> key;
  if (composedAfter < composedBefore) {
    key = "Backspace";
  } else if (after && composedAfter > composedBefore) {
    key = encodeUtf8(std::u32string_view(buffer_.text()).substr(after->end - 1, 1));
  }
  edited(key);
}

void TextInputView::imePanelHidden() {
  keyboardClosed_ = true;
}

} // namespace rnwebos
