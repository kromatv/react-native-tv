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
        TextMeasurement result{};
        result.size = measurer_->measure(
            attributedString, paragraphAttributes, layoutConstraints.maximumSize, layoutContext.pointScaleFactor);
        for (const auto &fragment : attributedString.getFragments()) {
          if (fragment.isAttachment()) {
            result.attachments.push_back({.frame = {}, .isClipped = true});
          }
        }
        return result;
      });
  measurement.size = layoutConstraints.clamp(measurement.size);
  return measurement;
}

} // namespace facebook::react
