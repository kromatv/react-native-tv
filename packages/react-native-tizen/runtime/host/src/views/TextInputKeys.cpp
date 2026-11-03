#include "TextInputView.h"

#include <algorithm>
#include <initializer_list>
#include <string_view>

namespace rntizen {

namespace {

constexpr char32_t kPasswordDot = U'•';
constexpr std::size_t kKeyPressUnits = 2;

std::string_view keyName(const Ecore_Event_Key &key) {
  return key.key != nullptr ? key.key : "";
}

bool isOneOf(std::string_view name, std::initializer_list<std::string_view> names) {
  for (auto candidate : names) {
    if (candidate == name) return true;
  }
  return false;
}

bool hasShift(const Ecore_Event_Key &key) {
  return (key.modifiers & ECORE_EVENT_MODIFIER_SHIFT) != 0;
}

std::u32string typedText(const Ecore_Event_Key &key) {
  constexpr unsigned int kCommandModifiers = ECORE_EVENT_MODIFIER_CTRL | ECORE_EVENT_MODIFIER_ALT | ECORE_EVENT_MODIFIER_WIN;
  if (key.string == nullptr || (key.modifiers & kCommandModifiers) != 0) return {};
  auto text = decodeUtf8(key.string);
  if (text.empty() || text.front() < 0x20 || text.front() == 0x7F) return {};
  return text;
}

std::optional<std::string> keyPressFor(std::u32string_view text) {
  if (text == U"\n") return "Enter";
  if (text.empty() || utf16Length(text) > kKeyPressUnits) return std::nullopt;
  return encodeUtf8(text);
}

} // namespace

bool TextInputView::keyDown(const Ecore_Event_Key &key, bool repeat) {
  const auto name = keyName(key);
  const bool extend = hasShift(key);
  if (name == "Select") {
    if (!repeat) inputMethodDone();
    return true;
  }
  if (isOneOf(name, {"Cancel", "XF86Back", "Escape"})) return true;
  if (isOneOf(name, {"Return", "KP_Enter"})) {
    if (submitBehavior() == facebook::react::SubmitBehavior::Newline) insert(U"\n");
    return true;
  }
  if (name == "BackSpace") {
    if (buffer_.deleteBackward()) {
      edited("Backspace");
    } else {
      emit("keyPress", folly::dynamic::object("key", "Backspace"));
    }
    return true;
  }
  if (isOneOf(name, {"Delete", "KP_Delete"})) {
    if (buffer_.deleteForward()) edited(std::nullopt);
    return true;
  }
  if (isOneOf(name, {"Left", "KP_Left"})) return stepCaret(-1, extend);
  if (isOneOf(name, {"Right", "KP_Right"})) return stepCaret(1, extend);
  if (isOneOf(name, {"Up", "KP_Up"})) return stepLine(-1, extend);
  if (isOneOf(name, {"Down", "KP_Down"})) return stepLine(1, extend);
  if (isOneOf(name, {"Home", "KP_Home"})) {
    moveCaret(0, extend);
    return true;
  }
  if (isOneOf(name, {"End", "KP_End"})) {
    moveCaret(buffer_.size(), extend);
    return true;
  }
  const auto text = typedText(key);
  if (text.empty()) return false;
  insert(text);
  return true;
}

void TextInputView::keyUp(const Ecore_Event_Key &key) {
  const auto name = keyName(key);
  if (isOneOf(name, {"Return", "KP_Enter"}) && submitBehavior() != facebook::react::SubmitBehavior::Newline) {
    returnPressed();
  } else if (isOneOf(name, {"XF86Back", "Escape"})) {
    backPressed();
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
  moveCaret(*landed, extend);
  return true;
}

bool TextInputView::leave() {
  blur();
  return false;
}

void TextInputView::returnPressed() {
  auto *focus = keyboard();
  if (panelClosed_ && focus != nullptr && focus->ime().isAvailable() && !focus->ime().isPanelShown()) {
    panelClosed_ = false;
    focus->ime().showPanel();
    return;
  }
  submit();
}

void TextInputView::backPressed() {
  auto *focus = keyboard();
  if (focus != nullptr && focus->ime().isPanelShown()) {
    focus->ime().hidePanel();
    return;
  }
  blur();
}

void TextInputView::inputMethodDone() {
  if (submitBehavior() != facebook::react::SubmitBehavior::Newline) {
    submit();
  } else if (auto *focus = keyboard()) {
    focus->ime().hidePanel();
  }
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
  const auto composedBefore = before ? before->end - before->start : 0;
  if (!buffer_.compose(decoded, static_cast<std::size_t>(std::max(0, cursor)), maxLength())) {
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

void TextInputView::imeDeleteSurrounding(int offset, int count) {
  if (count > 0 && buffer_.deleteAround(offset, static_cast<std::size_t>(count))) edited("Backspace");
}

std::string TextInputView::imeSurrounding(int &cursor) const {
  cursor = static_cast<int>(buffer_.caret());
  if (props() && inputProps().secureTextEntry) return encodeUtf8(std::u32string(buffer_.size(), kPasswordDot));
  return buffer_.utf8();
}

void TextInputView::imePanelHidden() {
  panelClosed_ = true;
}

} // namespace rntizen
