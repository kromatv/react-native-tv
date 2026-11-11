#include "SkiaTextMeasurer.h"

#include "ParagraphLayout.h"
#include "PreparedParagraph.h"

namespace rnwebos {

facebook::react::TextMeasurement SkiaTextMeasurer::measure(
    const facebook::react::AttributedString &attributedString,
    const facebook::react::ParagraphAttributes &paragraphAttributes,
    facebook::react::Size maximumSize,
    facebook::react::Float pointScaleFactor) const {
  const auto layout = layOutParagraph(attributedString, paragraphAttributes, maximumSize.width, pointScaleFactor);
  facebook::react::TextMeasurement measurement{};
  measurement.size = layout.size;
  for (const auto &frame : layout.attachments) measurement.attachments.push_back({.frame = frame, .isClipped = false});
  return measurement;
}

std::shared_ptr<const facebook::react::PreparedText> SkiaTextMeasurer::prepare(
    const facebook::react::AttributedString &attributedString,
    const facebook::react::ParagraphAttributes &paragraphAttributes,
    const facebook::react::LayoutConstraints &layoutConstraints,
    facebook::react::Float pointScaleFactor) const {
  return prepareParagraph(attributedString, paragraphAttributes, layoutConstraints, pointScaleFactor);
}

} // namespace rnwebos
