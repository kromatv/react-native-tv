#pragma once

#include "../fabric/TextInputShadowNode.h"
#include "../input/EditBuffer.h"
#include "../input/KeyboardFocus.h"
#include "ComponentView.h"
#include "TextInputLayer.h"

#include <memory>
#include <optional>
#include <string>

namespace rnwebos {

/// A `<TextInput>`, mounted as React Native's `AndroidTextInput`: its value,
/// or its placeholder, drawn like a `<Text>`, with a caret and a selection,
/// edited from the TV's on-screen keyboard or a keyboard's keys while it has
/// the keyboard, and reporting Android's events with Android's payloads and
/// event counts. `focus`, `blur` and `setTextAndSelection` are its commands.
class TextInputView final : public ComponentView, public KeyboardClient {
 public:
  TextInputView(Scene &scene, facebook::react::Tag tag);
  ~TextInputView() override;

  void updateProps(const facebook::react::Props::Shared &props) override;
  void updateState(const facebook::react::State::Shared &state) override;
  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;
  void handleCommand(const std::string &name, const folly::dynamic &args) override;

  bool keyDown(const SDL_KeyboardEvent &key, bool repeat) override;
  void keyUp(const SDL_KeyboardEvent &key) override;
  void keyboardLost() override;
  void imeCommit(std::string_view text) override;
  void imePreedit(std::string_view text, int cursor) override;
  void imePanelHidden() override;

 private:
  void drawContent(SkCanvas &canvas, float alpha) override;
  int contentLayers() const override;
  void layoutContent() override;
  void didMount() override;

  const TextInputProps &inputProps() const;
  KeyboardFocus *keyboard() const;
  bool isEditable() const;
  std::size_t maxLength() const;
  facebook::react::SubmitBehavior submitBehavior() const;

  void focus();
  void blur();
  void lostFocus();
  void insert(std::u32string_view text);
  void edited(std::optional<std::string> key);
  void moveCaret(std::size_t position, bool extend);
  bool stepCaret(int delta, bool extend);
  bool stepLine(int lines, bool extend);
  bool leave();
  bool reopensKeyboard() const;
  void okReleased();
  void backReleased();
  void submit();
  void setTextFromReact(const std::string &text);
  void pushTextToState(const std::string &text, int eventCount) const;

  void refresh();
  facebook::react::AttributedString shownText() const;
  std::size_t shownOffset(std::size_t position) const;
  PixelRect contentBox() const;
  void reportCaretToKeyboard();
  void emitSelection();
  void emitContentSize();
  void emit(std::string name, folly::dynamic payload) const;

  TextInputLayer layer_;
  EditBuffer buffer_;
  std::shared_ptr<const TextInputShadowNode::ConcreteState> state_;
  EditBuffer::Range reportedSelection_{};
  std::optional<facebook::react::Size> reportedContentSize_;
  int nativeEventCount_{0};
  bool focused_{false};
  bool keyboardClosed_{false};
  bool typedWithKeys_{false};
  bool autoFocused_{false};
};

} // namespace rnwebos
