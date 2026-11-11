#pragma once

#include <react/renderer/textlayoutmanager/TextMeasurer.h>

namespace rnwebos {

/// Measures paragraphs with SkParagraph, the engine ParagraphView draws with,
/// on the thread that asks: Fabric's layout thread, mostly.
class SkiaTextMeasurer final : public facebook::react::TextMeasurer {
 public:
  /// `pointScale` is the surface's device pixels per point, for the one call
  /// React Native makes without it.
  explicit SkiaTextMeasurer(facebook::react::Float pointScale = 1);

  facebook::react::TextMeasurement measure(
      const facebook::react::AttributedString &attributedString,
      const facebook::react::ParagraphAttributes &paragraphAttributes,
      facebook::react::Size maximumSize,
      facebook::react::Float pointScaleFactor) const override;

  std::shared_ptr<const facebook::react::PreparedText> prepare(
      const facebook::react::AttributedString &attributedString,
      const facebook::react::ParagraphAttributes &paragraphAttributes,
      const facebook::react::LayoutConstraints &layoutConstraints,
      facebook::react::Float pointScaleFactor) const override;

  facebook::react::LinesMeasurements measureLines(
      const facebook::react::AttributedString &attributedString,
      const facebook::react::ParagraphAttributes &paragraphAttributes,
      facebook::react::Size size) const override;

 private:
  const facebook::react::Float pointScale_;
};

} // namespace rnwebos
