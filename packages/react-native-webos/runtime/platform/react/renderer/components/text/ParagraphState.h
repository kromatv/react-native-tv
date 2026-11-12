#pragma once

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/textlayoutmanager/TextLayoutManager.h>
#include <react/renderer/textlayoutmanager/TextLayoutManagerExtended.h>

#include <memory>
#include <utility>

namespace facebook::react {

/// The state of a `<Paragraph>` on webOS: what React Native keeps for a C++
/// platform, plus the layout the shadow node prepared, as on Android, which
/// the view draws instead of laying the text out a second time.
class ParagraphState final {
 public:
  AttributedString attributedString;
  ParagraphAttributes paragraphAttributes;
  std::weak_ptr<const TextLayoutManager> layoutManager;
  /// Empty while React Native's `enablePreparedTextLayout` flag is off.
  MeasuredPreparedTextLayout measuredLayout;

  ParagraphState() = default;

  ParagraphState(
      AttributedString attributedString,
      ParagraphAttributes paragraphAttributes,
      std::weak_ptr<const TextLayoutManager> layoutManager,
      MeasuredPreparedTextLayout measuredLayout = {})
      : attributedString(std::move(attributedString)),
        paragraphAttributes(std::move(paragraphAttributes)),
        layoutManager(std::move(layoutManager)),
        measuredLayout(std::move(measuredLayout)) {}
};

} // namespace facebook::react
