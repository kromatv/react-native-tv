#include "TextLayoutManager.h"

#include <react/renderer/textlayoutmanager/TextMeasurer.h>

namespace facebook::react {

TextLayoutManager::TextLayoutManager(const std::shared_ptr<const ContextContainer> &contextContainer)
    : contextContainer_(contextContainer),
      measurer_(contextContainer->at<std::shared_ptr<const TextMeasurer>>(TextMeasurer::contextKey)) {}

TextMeasurement TextLayoutManager::measure(
    const AttributedStringBox &attributedStringBox,
    const ParagraphAttributes &paragraphAttributes,
    const TextLayoutContext &layoutContext,
    const LayoutConstraints &layoutConstraints) const {
  const auto &attributedString = attributedStringBox.getValue();
  auto measurement = textMeasureCache_.get(
      {.attributedString = attributedString,
       .paragraphAttributes = paragraphAttributes,
       .layoutConstraints = layoutConstraints,
       .pointScaleFactor = layoutContext.pointScaleFactor},
      [this, &attributedString, &paragraphAttributes, &layoutConstraints, &layoutContext] {
        return measurer_->measure(
            attributedString, paragraphAttributes, layoutConstraints.maximumSize, layoutContext.pointScaleFactor);
      });
  measurement.size = layoutConstraints.clamp(measurement.size);
  return measurement;
}

TextLayoutManager::PreparedTextLayout TextLayoutManager::prepareLayout(
    const AttributedString &attributedString,
    const ParagraphAttributes &paragraphAttributes,
    const TextLayoutContext &layoutContext,
    const LayoutConstraints &layoutConstraints) const {
  return preparedTextCache_.get(
      {.attributedString = attributedString,
       .paragraphAttributes = paragraphAttributes,
       .layoutConstraints = layoutConstraints,
       .pointScaleFactor = layoutContext.pointScaleFactor},
      [this, &attributedString, &paragraphAttributes, &layoutConstraints, &layoutContext] {
        return measurer_->prepare(
            attributedString, paragraphAttributes, layoutConstraints, layoutContext.pointScaleFactor);
      });
}

TextMeasurement TextLayoutManager::measurePreparedLayout(
    const PreparedTextLayout &layout,
    const TextLayoutContext &,
    const LayoutConstraints &layoutConstraints) const {
  auto measurement = layout->measurement;
  measurement.size = layoutConstraints.clamp(measurement.size);
  return measurement;
}

LinesMeasurements TextLayoutManager::measureLines(
    const AttributedStringBox &attributedStringBox,
    const ParagraphAttributes &paragraphAttributes,
    const Size &size) const {
  const auto &attributedString = attributedStringBox.getValue();
  return lineMeasureCache_.get(
      {.attributedString = attributedString, .paragraphAttributes = paragraphAttributes, .size = size},
      [this, &attributedString, &paragraphAttributes, &size] {
        return measurer_->measureLines(attributedString, paragraphAttributes, size);
      });
}

} // namespace facebook::react
