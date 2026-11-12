#pragma once

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/graphics/Size.h>

namespace facebook::react {

/// Measures a paragraph with the same engine that later draws it, so layout and
/// pixels agree: in points, laid out at `pointScaleFactor` device pixels per
/// point, as it will be drawn. The host registers one in the ContextContainer
/// under `TextMeasurer::contextKey`; it may be called from any thread.
class TextMeasurer {
 public:
  static constexpr const char *contextKey = "rn-tizen.TextMeasurer";

  virtual ~TextMeasurer() = default;

  virtual Size measure(
      const AttributedString &attributedString,
      const ParagraphAttributes &paragraphAttributes,
      Size maximumSize,
      Float pointScaleFactor) const = 0;
};

} // namespace facebook::react
