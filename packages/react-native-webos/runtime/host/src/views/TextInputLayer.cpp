#include "TextInputLayer.h"

#include "../text/TextEngine.h"

#include "include/core/SkPaint.h"
#include "modules/skparagraph/include/Metrics.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <mutex>

namespace rnwebos {

namespace {

using namespace skia::textlayout;

constexpr auto kBlinkPeriod = std::chrono::milliseconds(500);

int scrolledToShow(int scroll, int start, int size, int viewport, int content) {
  if (content <= viewport) return 0;
  if (start < scroll) scroll = start;
  if (start + size > scroll + viewport) scroll = start + size - viewport;
  return std::clamp(scroll, 0, content - viewport);
}

} // namespace

TextInputLayer::TextInputLayer(Scene &scene) : scene_(scene) {}

TextInputLayer::~TextInputLayer() {
  if (blink_ != 0) MainLoop::get().removeTimer(blink_);
}

void TextInputLayer::setText(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    bool multiline) {
  if (multiline == multiline_ && attributes == attributes_ && text == text_) return;
  text_ = text;
  attributes_ = attributes;
  multiline_ = multiline;
  layout_ = {};
  restartBlink();
}

void TextInputLayer::setSelection(std::size_t caret, std::size_t start, std::size_t end, SkColor4f color) {
  caret_ = caret;
  selectionStart_ = start;
  selectionEnd_ = end;
  selectionColor_ = color;
  placed_ = false;
  restartBlink();
}

void TextInputLayer::setVerticalAlignment(std::optional<facebook::react::TextAlignmentVertical> alignment) {
  if (alignment == verticalAlignment_) return;
  verticalAlignment_ = alignment;
  placed_ = false;
  scene_.invalidate();
}

void TextInputLayer::setCaret(bool shown, SkColor4f color) {
  caretColor_ = color;
  if (shown == caretShown_) return;
  caretShown_ = shown;
  restartBlink();
}

void TextInputLayer::layout(const PixelRect &box, facebook::react::Float scale, int caretWidth) {
  if (box.width != box_.width || scale != scale_) layout_ = {};
  box_ = box;
  scale_ = scale;
  caretWidth_ = std::max(1, caretWidth);
  placed_ = false;
  scene_.invalidate();
}

bool TextInputLayer::ensureLaidOut() {
  if (text_.isEmpty()) return false;
  if (!layout_.paragraph) {
    const auto boxWidth = std::max(1, box_.width);
    if (multiline_) {
      layout_ = layOutParagraph(text_, attributes_, static_cast<facebook::react::Float>(boxWidth) / scale_, scale_);
      textWidth_ = boxWidth;
    } else {
      layout_ = layOutParagraph(text_, attributes_, std::numeric_limits<facebook::react::Float>::infinity(), scale_);
      auto &paragraph = *layout_.paragraph;
      const auto unbroken = std::max(paragraph.getMaxIntrinsicWidth(), paragraph.getLongestLine());
      // One pixel more than the text needs: SkParagraph breaks a line that
      // only just fits, and a single line would lose what follows the break.
      textWidth_ = std::max(boxWidth, static_cast<int>(std::ceil(unbroken)) + 1);
      const std::lock_guard lock(textEngine().mutex);
      paragraph.layout(static_cast<float>(textWidth_));
    }
    textHeight_ = std::max(1, static_cast<int>(std::ceil(layout_.paragraph->getHeight())));
    readLines();
    placed_ = false;
  }
  if (!placed_) place();
  return true;
}

void TextInputLayer::readLines() {
  std::vector<LineMetrics> metrics;
  layout_.paragraph->getLineMetrics(metrics);
  lines_.clear();
  for (const auto &line : metrics) {
    const bool isLast = &line == &metrics.back();
    lines_.push_back({
        .start = line.fStartIndex,
        .end = line.fHardBreak || isLast ? line.fEndIndex : line.fEndExcludingWhitespaces,
        .left = static_cast<float>(line.fLeft),
        .top = static_cast<float>(line.fBaseline - line.fAscent),
        .height = static_cast<float>(line.fAscent + line.fDescent),
    });
  }
}

std::size_t TextInputLayer::lineOf(std::size_t offset) const {
  std::size_t found = 0;
  for (std::size_t index = 0; index < lines_.size(); ++index) {
    if (lines_[index].start <= offset) found = index;
  }
  return found;
}

float TextInputLayer::caretX(const Line &line, std::size_t offset) const {
  auto &paragraph = *layout_.paragraph;
  const auto start = static_cast<unsigned>(line.start);
  const auto at = static_cast<unsigned>(offset);
  if (offset > line.start) {
    // The whole line up to the caret, not the character before it alone:
    // SkParagraph gives no box to part of a grapheme.
    const auto before = paragraph.getRectsForRange(start, at, RectHeightStyle::kTight, RectWidthStyle::kTight);
    if (!before.empty()) {
      const auto &box = before.back();
      return box.direction == TextDirection::kRtl ? box.rect.left() : box.rect.right();
    }
  }
  const auto after = paragraph.getRectsForRange(at, at + 1, RectHeightStyle::kTight, RectWidthStyle::kTight);
  if (after.empty()) return line.left;
  const auto &box = after.front();
  return box.direction == TextDirection::kRtl ? box.rect.right() : box.rect.left();
}

void TextInputLayer::place() {
  using enum facebook::react::TextAlignmentVertical;
  placed_ = true;
  selectionRects_.clear();
  caretInText_ = SkRect::MakeWH(static_cast<float>(caretWidth_), static_cast<float>(textHeight_));
  if (!lines_.empty()) {
    const auto &line = lines_[lineOf(caret_)];
    caretInText_ = SkRect::MakeXYWH(
        std::round(caretX(line, caret_)), std::floor(line.top), static_cast<float>(caretWidth_), std::ceil(line.height));
  }
  if (selectionEnd_ > selectionStart_) {
    const auto boxes = layout_.paragraph->getRectsForRange(
        static_cast<unsigned>(selectionStart_),
        static_cast<unsigned>(selectionEnd_),
        RectHeightStyle::kMax,
        RectWidthStyle::kTight);
    for (const auto &box : boxes) selectionRects_.push_back(box.rect);
  }

  offsetY_ = 0;
  if (textHeight_ < box_.height) {
    const auto alignment = verticalAlignment_.value_or(Auto);
    if (alignment == Bottom) {
      offsetY_ = box_.height - textHeight_;
    } else if (alignment != Top) {
      offsetY_ = (box_.height - textHeight_) / 2;
    }
  }
  const auto caretLeft = static_cast<int>(caretInText_.left());
  const auto caretTop = static_cast<int>(caretInText_.top());
  if (multiline_) {
    scrollX_ = 0;
    scrollY_ = scrolledToShow(scrollY_, caretTop, static_cast<int>(caretInText_.height()), box_.height, textHeight_);
  } else {
    scrollY_ = 0;
    scrollX_ = scrolledToShow(scrollX_, caretLeft, caretWidth_, box_.width, textWidth_ + caretWidth_);
  }
}

void TextInputLayer::draw(SkCanvas &canvas) {
  if (box_.width <= 0 || box_.height <= 0 || !ensureLaidOut()) return;
  const SkAutoCanvasRestore restore(&canvas, true);
  canvas.clipRect(SkRect::MakeXYWH(
      static_cast<float>(box_.x),
      static_cast<float>(box_.y),
      static_cast<float>(box_.width + caretWidth_),
      static_cast<float>(box_.height)));
  canvas.translate(static_cast<float>(box_.x - scrollX_), static_cast<float>(box_.y + offsetY_ - scrollY_));
  if (!selectionRects_.empty()) {
    SkPaint highlight;
    highlight.setColor4f(selectionColor_);
    for (const auto &rect : selectionRects_) canvas.drawRect(rect, highlight);
  }
  layout_.paragraph->paint(&canvas, 0, 0);
  if (caretShown_ && caretLit_) {
    SkPaint caret;
    caret.setColor4f(caretColor_);
    canvas.drawRect(caretInText_, caret);
  }
}

PixelRect TextInputLayer::caretRect() {
  if (!ensureLaidOut()) return {.x = box_.x, .y = box_.y, .width = caretWidth_, .height = box_.height};
  return {
      .x = box_.x - scrollX_ + static_cast<int>(caretInText_.left()),
      .y = box_.y + offsetY_ - scrollY_ + static_cast<int>(caretInText_.top()),
      .width = caretWidth_,
      .height = static_cast<int>(caretInText_.height()),
  };
}

PixelSize TextInputLayer::textSize() {
  if (!ensureLaidOut()) return {};
  return {.width = textWidth_, .height = textHeight_};
}

std::optional<std::size_t> TextInputLayer::caretOnLine(int lines) {
  if (!multiline_ || !ensureLaidOut() || lines_.empty()) return std::nullopt;
  const auto target = static_cast<long>(lineOf(caret_)) + lines;
  if (target < 0 || target >= static_cast<long>(lines_.size())) return std::nullopt;
  const auto &line = lines_[static_cast<std::size_t>(target)];
  const auto hit = layout_.paragraph->getGlyphPositionAtCoordinate(caretInText_.left(), line.top + line.height / 2);
  const auto landed = static_cast<std::size_t>(std::max(0, hit.position));
  return std::clamp(landed, line.start, std::max(line.start, line.end));
}

void TextInputLayer::restartBlink() {
  caretLit_ = true;
  if (blink_ != 0) MainLoop::get().removeTimer(blink_);
  blink_ = 0;
  if (caretShown_) armBlink();
  scene_.invalidate();
}

void TextInputLayer::armBlink() {
  blink_ = MainLoop::get().addTimer(kBlinkPeriod, [this] {
    caretLit_ = !caretLit_;
    scene_.invalidate();
    armBlink();
  });
}

} // namespace rnwebos
