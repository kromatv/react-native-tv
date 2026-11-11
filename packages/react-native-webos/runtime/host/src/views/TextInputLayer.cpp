#include "TextInputLayer.h"

#include "EvasColor.h"

#include <algorithm>

namespace rntizen {

namespace {

constexpr double kBlinkSeconds = 0.5;
constexpr Evas_Coord kUnboundedWidth = 100000;

Evas_Coord scrolledToShow(Evas_Coord scroll, Evas_Coord start, Evas_Coord size, Evas_Coord viewport, Evas_Coord content) {
  if (content <= viewport) return 0;
  if (start < scroll) scroll = start;
  if (start + size > scroll + viewport) scroll = start + size - viewport;
  return std::clamp<Evas_Coord>(scroll, 0, content - viewport);
}

} // namespace

TextInputLayer::TextInputLayer(Evas *evas)
    : clip_(evas_object_rectangle_add(evas)),
      text_(evas_object_textblock_add(evas)),
      caret_(evas_object_rectangle_add(evas)),
      style_(evas_textblock_style_new()),
      cursor_(evas_object_textblock_cursor_new(text_)) {
  evas_object_textblock_style_set(text_, style_);
  for (auto *object : {clip_, text_, caret_}) evas_object_pass_events_set(object, EINA_TRUE);
}

TextInputLayer::~TextInputLayer() {
  if (blink_ != nullptr) ecore_timer_del(blink_);
  evas_textblock_cursor_free(cursor_);
  evas_object_del(caret_);
  evas_object_del(text_);
  evas_object_del(clip_);
  evas_textblock_style_free(style_);
}

Evas_Object *TextInputLayer::clip() const {
  return clip_;
}

Evas_Object *TextInputLayer::text() const {
  return text_;
}

Evas_Object *TextInputLayer::caret() const {
  return caret_;
}

void TextInputLayer::setContent(const TextblockContent &content, std::size_t caret, bool multiline) {
  evas_textblock_style_set(style_, content.style.c_str());
  evas_object_textblock_text_markup_set(text_, content.markup.c_str());
  caretIndex_ = caret;
  multiline_ = multiline;
  layout(box_, caretWidth_);
  restartBlink();
}

void TextInputLayer::setVerticalAlignment(std::optional<facebook::react::TextAlignmentVertical> alignment) {
  verticalAlignment_ = alignment;
}

void TextInputLayer::setCaret(bool shown, const facebook::react::SharedColor &color) {
  setColor(caret_, color);
  caretShown_ = shown;
  restartBlink();
}

void TextInputLayer::layout(const PixelRect &box, Evas_Coord caretWidth) {
  using enum facebook::react::TextAlignmentVertical;
  box_ = box;
  caretWidth_ = std::max<Evas_Coord>(1, caretWidth);
  setGeometry(clip_, inflated(box, 0, 0, caretWidth_, 0));
  Evas_Coord width = 0;
  Evas_Coord height = 0;
  if (multiline_) {
    evas_object_resize(text_, std::max<Evas_Coord>(1, box.width), std::max<Evas_Coord>(1, box.height));
    evas_object_textblock_size_formatted_get(text_, nullptr, &height);
    width = std::max<Evas_Coord>(1, box.width);
  } else {
    evas_object_resize(text_, kUnboundedWidth, std::max<Evas_Coord>(1, box.height));
    evas_object_textblock_size_native_get(text_, &width, &height);
    width = std::max(width, box.width);
  }
  textWidth_ = width;
  textHeight_ = std::max<Evas_Coord>(1, height);
  evas_object_resize(text_, textWidth_, textHeight_);

  evas_textblock_cursor_pos_set(cursor_, static_cast<int>(caretIndex_));
  Evas_Coord x = 0;
  Evas_Coord y = 0;
  Evas_Coord h = 0;
  if (evas_textblock_cursor_geometry_get(cursor_, &x, &y, nullptr, &h, nullptr, EVAS_TEXTBLOCK_CURSOR_BEFORE) < 0 ||
      h <= 0) {
    x = 0;
    y = 0;
    h = textHeight_;
  }
  caretInText_ = {.x = x, .y = y, .width = caretWidth_, .height = h};

  Evas_Coord offsetY = 0;
  if (textHeight_ < box.height) {
    const auto alignment = verticalAlignment_.value_or(Auto);
    if (alignment == Bottom) {
      offsetY = box.height - textHeight_;
    } else if (alignment != Top) {
      offsetY = (box.height - textHeight_) / 2;
    }
  }
  if (multiline_) {
    scrollX_ = 0;
    scrollY_ = scrolledToShow(scrollY_, y, h, box.height, textHeight_);
  } else {
    scrollY_ = 0;
    scrollX_ = scrolledToShow(scrollX_, x, caretWidth_, box.width, textWidth_ + caretWidth_);
  }
  setGeometry(text_, {.x = box.x - scrollX_, .y = box.y + offsetY - scrollY_, .width = textWidth_, .height = textHeight_});
  placeCaret();
}

PixelRect TextInputLayer::caretRect() const {
  Evas_Coord x = 0;
  Evas_Coord y = 0;
  evas_object_geometry_get(text_, &x, &y, nullptr, nullptr);
  return {.x = x + caretInText_.x, .y = y + caretInText_.y, .width = caretInText_.width, .height = caretInText_.height};
}

Evas_Coord TextInputLayer::textWidth() const {
  return textWidth_;
}

Evas_Coord TextInputLayer::textHeight() const {
  return textHeight_;
}

std::optional<std::size_t> TextInputLayer::caretOnLine(int lines) const {
  if (!multiline_) return std::nullopt;
  const auto y = lines < 0 ? caretInText_.y - 1 : caretInText_.y + caretInText_.height + 1;
  if (y < 0 || y >= textHeight_) return std::nullopt;
  auto *probe = evas_object_textblock_cursor_new(text_);
  std::optional<std::size_t> landed;
  Evas_Coord charX = 0;
  Evas_Coord charWidth = 0;
  if (evas_textblock_cursor_char_coord_set(probe, caretInText_.x, y) == EINA_TRUE) {
    evas_textblock_cursor_char_geometry_get(probe, &charX, nullptr, &charWidth, nullptr);
    const auto position = static_cast<std::size_t>(std::max(0, evas_textblock_cursor_pos_get(probe)));
    landed = caretInText_.x > charX + charWidth / 2 ? position + 1 : position;
  } else if (evas_textblock_cursor_line_coord_set(probe, y) >= 0) {
    evas_textblock_cursor_line_char_last(probe);
    const auto position = static_cast<std::size_t>(std::max(0, evas_textblock_cursor_pos_get(probe)));
    landed = evas_textblock_cursor_is_format(probe) == EINA_TRUE ? position : position + 1;
  }
  evas_textblock_cursor_free(probe);
  return landed;
}

void TextInputLayer::restartBlink() {
  caretLit_ = true;
  if (blink_ != nullptr) {
    ecore_timer_del(blink_);
    blink_ = nullptr;
  }
  if (caretShown_) blink_ = ecore_timer_add(kBlinkSeconds, onBlink, this);
  placeCaret();
}

Eina_Bool TextInputLayer::onBlink(void *self) {
  auto &layer = *static_cast<TextInputLayer *>(self);
  layer.caretLit_ = !layer.caretLit_;
  layer.placeCaret();
  return ECORE_CALLBACK_RENEW;
}

void TextInputLayer::placeCaret() {
  setGeometry(caret_, caretRect());
  if (caretShown_ && caretLit_) {
    evas_object_show(caret_);
  } else {
    evas_object_hide(caret_);
  }
}

} // namespace rntizen
