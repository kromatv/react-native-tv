#include "ParagraphLayout.h"

#include "../views/SkiaColor.h"
#include "TextCase.h"
#include "TextEngine.h"
#include "TextStyles.h"

#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"

#include <algorithm>
#include <cmath>
#include <optional>

namespace rnwebos {

namespace {

using facebook::react::Float;
using facebook::react::TextAlignment;
using namespace skia::textlayout;

// Wide enough for any line, small enough that float positions stay exact.
constexpr float kUnboundedWidth = 100000;

TextAlign textAlign(std::optional<TextAlignment> alignment) {
  using enum TextAlignment;
  switch (alignment.value_or(Natural)) {
    case Left:
      return TextAlign::kLeft;
    case Center:
      return TextAlign::kCenter;
    case Right:
      return TextAlign::kRight;
    case Justified:
      return TextAlign::kJustify;
    case End:
      return TextAlign::kEnd;
    case Natural:
    case Start:
      return TextAlign::kStart;
  }
  return TextAlign::kStart;
}

ParagraphStyle paragraphStyle(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes) {
  ParagraphStyle style;
  const auto &fragments = text.getFragments();
  if (!fragments.empty()) {
    const auto &first = fragments.front().textAttributes;
    style.setTextAlign(textAlign(first.alignment));
    const bool rightToLeft = first.baseWritingDirection == facebook::react::WritingDirection::RightToLeft ||
        (first.baseWritingDirection != facebook::react::WritingDirection::LeftToRight &&
         first.layoutDirection == facebook::react::LayoutDirection::RightToLeft);
    style.setTextDirection(rightToLeft ? TextDirection::kRtl : TextDirection::kLtr);
  }
  if (attributes.maximumNumberOfLines > 0) {
    style.setMaxLines(static_cast<std::size_t>(attributes.maximumNumberOfLines));
    if (attributes.ellipsizeMode != facebook::react::EllipsizeMode::Clip) style.setEllipsis(u"…");
  }
  return style;
}

float widthLimit(Float maximumWidth, Float scale) {
  const auto pixels = maximumWidth * scale;
  if (std::isnan(pixels) || std::isinf(pixels) || pixels > kUnboundedWidth) return kUnboundedWidth;
  return std::max(1.0f, std::floor(static_cast<float>(pixels)));
}

// The width of the box a paragraph laid out within `limit` ends up in: its
// widest line's, at least the minimum.
float boxWidth(Paragraph &paragraph, Float minimumWidth, Float scale, float limit) {
  const auto least = std::min(limit, std::ceil(static_cast<float>(std::max<Float>(0, minimumWidth) * scale)));
  return std::clamp(std::ceil(std::max(0.0f, paragraph.getLongestLine())), std::max(1.0f, least), limit);
}

ParagraphLayout layOut(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    std::optional<Float> minimumWidth,
    Float maximumWidth,
    Float scale) {
  const auto pixelScale = scale > 0 ? scale : 1;
  auto &engine = textEngine();
  std::lock_guard lock(engine.mutex);
  const auto style = paragraphStyle(text, attributes);
  auto builder = ParagraphBuilder::make(style, engine.fonts, engine.unicode);
  std::size_t attachmentCount = 0;
  for (const auto &fragment : text.getFragments()) {
    if (fragment.isAttachment()) {
      const auto &size = fragment.parentShadowView.layoutMetrics.frame.size;
      builder->addPlaceholder(PlaceholderStyle(
          static_cast<float>(size.width * pixelScale),
          static_cast<float>(size.height * pixelScale),
          PlaceholderAlignment::kAboveBaseline,
          TextBaseline::kAlphabetic,
          0));
      ++attachmentCount;
      continue;
    }
    builder->pushStyle(textStyle(fragment.textAttributes, pixelScale));
    const auto shown = transformedText(
        fragment.string, fragment.textAttributes.textTransform.value_or(facebook::react::TextTransform::None));
    builder->addText(shown.data(), shown.size());
    builder->pop();
  }
  ParagraphLayout layout;
  layout.paragraph = builder->Build();
  const auto limit = widthLimit(maximumWidth, pixelScale);
  layout.paragraph->layout(limit);
  // Left-aligned lines sit the same in any box; the others move with its width.
  if (minimumWidth && style.effective_align() != TextAlign::kLeft) {
    const auto box = boxWidth(*layout.paragraph, *minimumWidth, pixelScale, limit);
    if (box < limit) layout.paragraph->layout(box);
  }
  // SkParagraph answers a huge negative longest line for a text with no glyphs.
  layout.size = {
      .width = std::ceil(std::max(0.0f, layout.paragraph->getLongestLine())) / pixelScale,
      .height = std::ceil(layout.paragraph->getHeight()) / pixelScale,
  };
  if (attachmentCount > 0) {
    for (const auto &box : layout.paragraph->getRectsForPlaceholders()) {
      layout.attachments.push_back({
          .origin = {.x = box.rect.left() / pixelScale, .y = box.rect.top() / pixelScale},
          .size = {.width = box.rect.width() / pixelScale, .height = box.rect.height() / pixelScale},
      });
    }
  }
  return layout;
}

} // namespace

ParagraphLayout layOutParagraph(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    Float maximumWidth,
    Float scale) {
  return layOut(text, attributes, std::nullopt, maximumWidth, scale);
}

ParagraphLayout layOutParagraphInBox(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    Float minimumWidth,
    Float maximumWidth,
    Float scale) {
  return layOut(text, attributes, minimumWidth, maximumWidth, scale);
}

} // namespace rnwebos
