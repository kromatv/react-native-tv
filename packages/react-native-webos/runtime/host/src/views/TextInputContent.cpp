#include "SkiaColor.h"
#include "TextInputView.h"

#include <algorithm>

namespace rnwebos {

using namespace facebook::react;

namespace {

constexpr Float kCaretWidth = 2;
constexpr float kSelectionOpacity = 0.35f;
constexpr char32_t kPasswordDot = U'\u2022';
constexpr const char *kEmptyLine = "\u200b";

SkColor4f colorOr(const SharedColor &color, const SharedColor &fallback, float opacity = 1) {
  if (color) return skColor(color);
  if (fallback) return skColor(fallback, opacity);
  return {0, 0, 0, opacity};
}

} // namespace

PixelRect TextInputView::contentBox() const {
  const auto scale = pointScale();
  const auto content = layoutMetrics().getContentFrame();
  return {
      .x = toPixels(content.origin.x, scale),
      .y = toPixels(content.origin.y, scale),
      .width = toPixels(content.size.width, scale),
      .height = toPixels(content.size.height, scale),
  };
}

void TextInputView::layoutContent() {
  const auto scale = pointScale();
  layer_.layout(contentBox(), scale, toPixels(kCaretWidth, scale));
  reportCaretToKeyboard();
}

int TextInputView::contentLayers() const {
  // The text's runs, the selection and the caret carry their own colors, so
  // the view's opacity needs a layer.
  return 2;
}

void TextInputView::drawContent(SkCanvas &canvas, float) {
  layer_.draw(canvas);
}

void TextInputView::refresh() {
  if (!props()) return;
  const auto &props = inputProps();
  const auto textColor = props.getEffectiveTextAttributes(1).foregroundColor;
  auto paragraph = props.paragraphAttributes;
  paragraph.maximumNumberOfLines = props.multiline ? 0 : 1;
  paragraph.ellipsizeMode = EllipsizeMode::Clip;
  const bool selecting = focused_ && buffer_.hasSelection() && !buffer_.composing();
  const auto selection = selecting ? buffer_.selection() : EditBuffer::Range{};
  layer_.setVerticalAlignment(props.paragraphAttributes.textAlignVertical);
  layer_.setText(shownText(), paragraph, props.multiline);
  layer_.setSelection(
      shownOffset(buffer_.caret()),
      shownOffset(selection.start),
      shownOffset(selection.end),
      colorOr(props.selectionColor, textColor, kSelectionOpacity));
  layer_.setCaret(focused_ && !props.caretHidden && !buffer_.hasSelection(), colorOr(props.cursorColor, textColor));
  reportCaretToKeyboard();
}

AttributedString TextInputView::shownText() const {
  const auto &props = inputProps();
  const auto base = props.getEffectiveTextAttributes(1);
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
    return shown;
  }
  const auto text = props.secureTextEntry ? std::u32string(buffer_.size(), kPasswordDot) : buffer_.text();
  const auto composing = buffer_.composing().value_or(EditBuffer::Range{});
  auto composed = base;
  composed.textDecorationLineType = TextDecorationLineType::Underline;
  const std::u32string_view view(text);
  shown.appendFragment(
      {.string = encodeUtf8(view.substr(0, composing.start)), .textAttributes = base, .parentShadowView = {}});
  shown.appendFragment(
      {.string = encodeUtf8(view.substr(composing.start, composing.end - composing.start)),
       .textAttributes = composed,
       .parentShadowView = {}});
  shown.appendFragment({.string = encodeUtf8(view.substr(composing.end)), .textAttributes = base, .parentShadowView = {}});
  return shown;
}

std::size_t TextInputView::shownOffset(std::size_t position) const {
  if (buffer_.size() == 0) return 0;
  return inputProps().secureTextEntry ? std::min(position, buffer_.size()) : buffer_.utf16Offset(position);
}

void TextInputView::reportCaretToKeyboard() {
  auto *focus = keyboard();
  if (focus == nullptr || !focused_) return;
  const auto view = canvasRect();
  const auto box = contentBox();
  const auto caret = layer_.caretRect();
  focus->ime().caretMoved(
      {.x = view.x + box.x, .y = view.y + caret.y, .width = box.width, .height = caret.height}, view.x + caret.x);
}

void TextInputView::emitContentSize() {
  const auto scale = pointScale();
  const auto &metrics = layoutMetrics();
  if (metrics.frame.size.width <= 0 || !props()) return;
  const auto &insets = metrics.contentInsets;
  const auto text = layer_.textSize();
  const Size size{
      .width = static_cast<Float>(text.width) / scale + insets.left + insets.right,
      .height = static_cast<Float>(text.height) / scale + insets.top + insets.bottom};
  if (reportedContentSize_ == size) return;
  reportedContentSize_ = size;
  emit(
      "contentSizeChange",
      folly::dynamic::object("contentSize", folly::dynamic::object("width", size.width)("height", size.height))(
          "target", tag()));
}

} // namespace rnwebos
