#include "TextInputView.h"

#include "../mounting/ComponentViewRegistry.h"

#include <algorithm>

namespace rntizen {

using namespace facebook::react;

namespace {

constexpr Float kCaretWidth = 2;
constexpr char32_t kPasswordDot = U'\u2022';
constexpr const char *kEmptyLine = "\u200b";
constexpr int kUnset = -1;

} // namespace

TextInputView::TextInputView(Evas *evas, Tag tag) : ComponentView(evas, tag), layer_(evas) {
  addMember(layer_.clip(), true);
  addMember(layer_.text(), false);
  addMember(layer_.caret(), false);
  evas_object_clip_set(layer_.text(), layer_.clip());
  evas_object_clip_set(layer_.caret(), layer_.clip());
}

TextInputView::~TextInputView() {
  if (auto *focus = keyboard(); focus != nullptr && focused_) focus->blur(*this);
}

const TextInputProps &TextInputView::inputProps() const {
  return static_cast<const TextInputProps &>(*props());
}

KeyboardFocus *TextInputView::keyboard() const {
  return KeyboardFocus::of(evas());
}

bool TextInputView::isEditable() const {
  return props() && inputProps().editable && !inputProps().readOnly;
}

std::size_t TextInputView::maxLength() const {
  return static_cast<std::size_t>(std::max(0, inputProps().maxLength));
}

SubmitBehavior TextInputView::submitBehavior() const {
  return inputProps().getNonDefaultSubmitBehavior();
}

void TextInputView::updateProps(const Props::Shared &props) {
  ComponentView::updateProps(props);
  if (focused_ && !isEditable()) blur();
  refresh();
}

void TextInputView::updateState(const State::Shared &state) {
  if (!state) return;
  state_ = std::static_pointer_cast<const TextInputShadowNode::ConcreteState>(state);
  const auto &data = state_->getData();
  if (data.mostRecentEventCount < nativeEventCount_) return;
  setTextFromReact(data.attributedStringBox.getValue().getString());
}

void TextInputView::updateLayout(const LayoutMetrics &layoutMetrics) {
  const bool rescaled = layoutMetrics.pointScaleFactor != contentScale_;
  ComponentView::updateLayout(layoutMetrics);
  if (rescaled) refresh();
  emitContentSize();
}

void TextInputView::handleCommand(const std::string &name, const folly::dynamic &args) {
  if (name == "focus") {
    focus();
  } else if (name == "blur") {
    blur();
  } else if (name == "setTextAndSelection" && args.isArray() && args.size() >= 4) {
    const auto eventCount = static_cast<int>(args[0].asInt());
    if (eventCount == kUnset || eventCount < nativeEventCount_) return;
    if (args[1].isString()) setTextFromReact(args[1].asString());
    const auto start = static_cast<int>(args[2].asInt());
    const auto end = args[3].asInt() == kUnset ? start : static_cast<int>(args[3].asInt());
    if (start == kUnset || end == kUnset) return;
    buffer_.selectUtf16(static_cast<std::size_t>(std::max(0, start)), static_cast<std::size_t>(std::max(0, end)));
    if (auto *focus = keyboard(); focus != nullptr && focused_) focus->ime().reset();
    emitSelection();
    refresh();
  }
}

void TextInputView::didMount() {
  if (autoFocused_ || !props() || !inputProps().autoFocus) return;
  autoFocused_ = true;
  focus();
}

void TextInputView::focus() {
  auto *focus = keyboard();
  if (focus == nullptr || focused_ || !isEditable()) return;
  const auto &props = inputProps();
  focused_ = true;
  panelClosed_ = false;
  auto options = imeOptionsFor(
      props.keyboardType, props.returnKeyType, props.autoCapitalize, props.autoCorrect, props.secureTextEntry,
      props.multiline);
  options.showPanel = props.showSoftInputOnFocus.value_or(true);
  focus->focus(*this, options);
  emit("focus", folly::dynamic::object("target", tag()));
  if (props.selectTextOnFocus) buffer_.selectAll();
  emitSelection();
  refresh();
}

void TextInputView::blur() {
  if (!focused_) return;
  if (auto *focus = keyboard()) focus->blur(*this);
  lostFocus();
}

void TextInputView::keyboardLost() {
  if (focused_) lostFocus();
}

void TextInputView::lostFocus() {
  focused_ = false;
  buffer_.finishComposing();
  emit("blur", folly::dynamic::object("target", tag()));
  emit("endEditing", folly::dynamic::object("target", tag())("text", buffer_.utf8()));
  refresh();
}

void TextInputView::edited(std::optional<std::string> key) {
  if (key) emit("keyPress", folly::dynamic::object("key", *key));
  const auto text = buffer_.utf8();
  pushTextToState(text, ++nativeEventCount_);
  const auto selection = buffer_.selectionUtf16();
  emit(
      "change",
      folly::dynamic::object("text", text)("eventCount", ++nativeEventCount_)("target", tag())(
          "selection", folly::dynamic::object("start", selection.start)("end", selection.end)));
  emitSelection();
  refresh();
  emitContentSize();
}

void TextInputView::submit() {
  const auto &returnKeyType = inputProps().returnKeyType;
  const auto *action = "submit";
  if (returnKeyType == "next" || returnKeyType == "previous") action = returnKeyType.c_str();
  emit("submitEditing", folly::dynamic::object("target", tag())("text", buffer_.utf8())("action", action));
  if (submitBehavior() == SubmitBehavior::BlurAndSubmit) blur();
}

void TextInputView::setTextFromReact(const std::string &text) {
  if (text == buffer_.utf8()) return;
  buffer_.setText(text);
  if (auto *focus = keyboard(); focus != nullptr && focused_) focus->ime().reset();
  emitSelection();
  refresh();
  emitContentSize();
}

void TextInputView::pushTextToState(const std::string &text, int eventCount) const {
  if (!state_) return;
  auto attributes = inputProps().getEffectiveTextAttributes(1);
  state_->updateState([text, attributes, eventCount](const TextInputState &previous) -> StateData::Shared {
    AttributedString attributedString;
    attributedString.setBaseTextAttributes(attributes);
    attributedString.appendFragment({.string = text, .textAttributes = attributes, .parentShadowView = {}});
    return std::make_shared<const TextInputState>(
        AttributedStringBox(attributedString), previous.reactTreeAttributedString, previous.paragraphAttributes, eventCount);
  });
}

void TextInputView::layoutMembers() {
  const auto &box = canvasRect();
  const auto scale = pointScale();
  const auto content = layoutMetrics().getContentFrame();
  layer_.layout(
      {.x = box.x + toPixels(content.origin.x, scale),
       .y = box.y + toPixels(content.origin.y, scale),
       .width = toPixels(content.size.width, scale),
       .height = toPixels(content.size.height, scale)},
      toPixels(kCaretWidth, scale));
  reportCaretToInputMethod();
}

void TextInputView::refresh() {
  if (!props()) return;
  const auto &props = inputProps();
  contentScale_ = layoutMetrics().pointScaleFactor;
  const auto caretColor = props.cursorColor ? props.cursorColor : props.getEffectiveTextAttributes(1).foregroundColor;
  layer_.setVerticalAlignment(props.paragraphAttributes.textAlignVertical);
  layer_.setCaret(focused_ && !props.caretHidden && !buffer_.hasSelection(), caretColor);
  layer_.setContent(content(), buffer_.size() == 0 ? 0 : buffer_.caret(), props.multiline);
  reportCaretToInputMethod();
}

TextblockContent TextInputView::content() const {
  const auto &props = inputProps();
  const auto base = props.getEffectiveTextAttributes(1);
  auto paragraph = props.paragraphAttributes;
  paragraph.maximumNumberOfLines = props.multiline ? 0 : 1;
  paragraph.ellipsizeMode = EllipsizeMode::Clip;
  AttributedString shown;
  if (buffer_.size() == 0) {
    auto attributes = base;
    if (props.placeholderTextColor) {
      attributes.foregroundColor = props.placeholderTextColor;
    } else {
      attributes.opacity = 0.5;
    }
    const auto &placeholder = props.placeholder.empty() ? std::string(kEmptyLine) : props.placeholder;
    shown.appendFragment({.string = placeholder, .textAttributes = attributes, .parentShadowView = {}});
    return textblockContent(shown, paragraph, pointScale());
  }
  const auto text = props.secureTextEntry ? std::u32string(buffer_.size(), kPasswordDot) : buffer_.text();
  auto marked = base;
  EditBuffer::Range range{};
  if (const auto composing = buffer_.composing()) {
    range = *composing;
    marked.textDecorationLineType = TextDecorationLineType::Underline;
  } else if (focused_ && buffer_.hasSelection()) {
    range = buffer_.selection();
    marked.backgroundColor = props.selectionColor
        ? props.selectionColor
        : colorFromComponents([&] {
            auto components = colorComponentsFromColor(base.foregroundColor);
            components.alpha *= 0.35f;
            return components;
          }());
  }
  const std::u32string_view view(text);
  shown.appendFragment({.string = encodeUtf8(view.substr(0, range.start)), .textAttributes = base, .parentShadowView = {}});
  shown.appendFragment(
      {.string = encodeUtf8(view.substr(range.start, range.end - range.start)),
       .textAttributes = marked,
       .parentShadowView = {}});
  shown.appendFragment({.string = encodeUtf8(view.substr(range.end)), .textAttributes = base, .parentShadowView = {}});
  return textblockContent(shown, paragraph, pointScale());
}

void TextInputView::reportCaretToInputMethod() {
  auto *focus = keyboard();
  if (focus == nullptr || !focused_) return;
  const auto caret = layer_.caretRect();
  focus->ime().caretMoved(static_cast<int>(buffer_.caret()), caret.x, caret.y, caret.width, caret.height);
}

void TextInputView::emitSelection() {
  const auto selection = buffer_.selectionUtf16();
  if (selection == reportedSelection_) return;
  reportedSelection_ = selection;
  emit(
      "selectionChange",
      folly::dynamic::object("selection", folly::dynamic::object("start", selection.start)("end", selection.end)));
}

void TextInputView::emitContentSize() {
  const auto scale = pointScale();
  const auto &metrics = layoutMetrics();
  if (metrics.frame.size.width <= 0 || !props()) return;
  const auto &insets = metrics.contentInsets;
  const Size size{
      .width = static_cast<Float>(layer_.textWidth()) / scale + insets.left + insets.right,
      .height = static_cast<Float>(layer_.textHeight()) / scale + insets.top + insets.bottom};
  if (reportedContentSize_ == size) return;
  reportedContentSize_ = size;
  emit(
      "contentSizeChange",
      folly::dynamic::object("contentSize", folly::dynamic::object("width", size.width)("height", size.height))(
          "target", tag()));
}

void TextInputView::emit(std::string name, folly::dynamic payload) const {
  if (const auto &emitter = eventEmitter()) emitter->dispatchEvent(std::move(name), std::move(payload));
}

RN_TIZEN_COMPONENT_VIEW("AndroidTextInput", TextInputView);

} // namespace rntizen
