#include "ParagraphLines.h"

#include "ParagraphLayout.h"
#include "TextCase.h"

#include "include/core/SkFontMetrics.h"
#include "modules/skparagraph/include/Metrics.h"

#include <cstddef>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

using facebook::react::Float;

// What SkParagraph puts in its text for an inline view.
constexpr const char *kObjectReplacement = "\xEF\xBF\xBC";

std::string shownText(const facebook::react::AttributedString &text) {
  std::string shown;
  for (const auto &fragment : text.getFragments()) {
    shown += fragment.isAttachment()
        ? kObjectReplacement
        : transformedText(
              fragment.string, fragment.textAttributes.textTransform.value_or(facebook::react::TextTransform::None));
  }
  return shown;
}

// Where each UTF-16 code unit of `utf8` starts in it: SkParagraph counts a
// line's ends in UTF-16 (docs/reference/skia-rendering.md, section 6).
std::vector<std::size_t> utf16Offsets(const std::string &utf8) {
  std::vector<std::size_t> offsets;
  for (std::size_t index = 0; index < utf8.size();) {
    const auto lead = static_cast<unsigned char>(utf8[index]);
    const std::size_t bytes = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    offsets.push_back(index);
    if (bytes == 4) offsets.push_back(index);
    index += bytes;
  }
  offsets.push_back(utf8.size());
  return offsets;
}

std::string between(const std::string &utf8, const std::vector<std::size_t> &offsets, std::size_t start, std::size_t end) {
  if (start >= end || end >= offsets.size()) return {};
  return utf8.substr(offsets[start], offsets[end] - offsets[start]);
}

} // namespace

facebook::react::LinesMeasurements measureParagraphLines(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    Float width,
    Float scale) {
  const auto pixelScale = scale > 0 ? scale : 1;
  const auto points = [pixelScale](double pixels) { return static_cast<Float>(pixels / pixelScale); };
  const auto layout = layOutParagraph(text, attributes, width, pixelScale);
  std::vector<skia::textlayout::LineMetrics> lines;
  layout.paragraph->getLineMetrics(lines);
  const auto shown = shownText(text);
  const auto offsets = utf16Offsets(shown);
  facebook::react::LinesMeasurements measurements;
  measurements.reserve(lines.size());
  double top = 0;
  for (const auto &line : lines) {
    SkFontMetrics font{};
    layout.paragraph->getFontAtUTF16Offset(line.fStartIndex).getMetrics(&font);
    measurements.emplace_back(
        between(shown, offsets, line.fStartIndex, line.fEndExcludingWhitespaces),
        facebook::react::Rect{
            .origin = {.x = points(line.fLeft), .y = points(top)},
            .size = {.width = points(line.fWidth), .height = points(line.fHeight)}},
        points(line.fDescent),
        points(font.fCapHeight),
        points(measurements.empty() ? layout.paragraph->getAlphabeticBaseline() : line.fBaseline - top),
        points(font.fXHeight));
    top += line.fHeight;
  }
  return measurements;
}

} // namespace rnwebos
