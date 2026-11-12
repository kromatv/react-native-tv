#include "PreparedParagraph.h"

#include "ParagraphLayout.h"

#include "include/core/SkPictureRecorder.h"

#include <utility>

namespace rnwebos {

namespace {

// What a paragraph draws outside its lines (a shadow, a slanted glyph) is not
// known before it is painted, so the recording is not culled to its box.
constexpr SkRect kRecordingBounds = SkRect::MakeLTRB(-100000, -100000, 100000, 100000);

} // namespace

PreparedParagraph::PreparedParagraph(
    facebook::react::TextMeasurement measurement,
    sk_sp<SkPicture> picture,
    facebook::react::Float scale)
    : PreparedText(std::move(measurement)), picture(std::move(picture)), scale(scale) {}

std::shared_ptr<const PreparedParagraph> prepareParagraph(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    const facebook::react::LayoutConstraints &constraints,
    facebook::react::Float scale) {
  const auto layout =
      layOutParagraphInBox(text, attributes, constraints.minimumSize.width, constraints.maximumSize.width, scale);
  SkPictureRecorder recorder;
  layout.paragraph->paint(recorder.beginRecording(kRecordingBounds), 0, 0);
  facebook::react::TextMeasurement measurement{};
  measurement.size = layout.size;
  for (const auto &frame : layout.attachments) measurement.attachments.push_back({.frame = frame, .isClipped = false});
  return std::make_shared<const PreparedParagraph>(
      std::move(measurement), recorder.finishRecordingAsPicture(), scale > 0 ? scale : 1);
}

} // namespace rnwebos
