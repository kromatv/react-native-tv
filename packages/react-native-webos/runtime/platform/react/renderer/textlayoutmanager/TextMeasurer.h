#pragma once

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/graphics/Size.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>

#include <memory>
#include <utility>

namespace facebook::react {

/// A paragraph laid out once for the box it is shown in, which its view draws
/// as it is. The host's measurer makes its own kind, which its views read.
class PreparedText {
 public:
  explicit PreparedText(TextMeasurement measurement) : measurement(std::move(measurement)) {}
  virtual ~PreparedText() = default;

  /// The text's size and where its inline views sit, in points.
  const TextMeasurement measurement;
};

/// Measures a paragraph with the same engine that later draws it, so layout and
/// pixels agree: its size and where its inline views sit, in points, laid out
/// at `pointScaleFactor` device pixels per point, as it will be drawn. The host registers one in the ContextContainer
/// under `TextMeasurer::contextKey`; it may be called from any thread.
class TextMeasurer {
 public:
  static constexpr const char *contextKey = "rn-webos.TextMeasurer";

  virtual ~TextMeasurer() = default;

  virtual TextMeasurement measure(
      const AttributedString &attributedString,
      const ParagraphAttributes &paragraphAttributes,
      Size maximumSize,
      Float pointScaleFactor) const = 0;

  /// Lays the paragraph out for a box whose width the constraints bound, ready
  /// to draw. A measurer that only measures leaves the drawing to the view.
  virtual std::shared_ptr<const PreparedText> prepare(
      const AttributedString &attributedString,
      const ParagraphAttributes &paragraphAttributes,
      const LayoutConstraints &layoutConstraints,
      Float pointScaleFactor) const {
    return std::make_shared<const PreparedText>(
        measure(attributedString, paragraphAttributes, layoutConstraints.maximumSize, pointScaleFactor));
  }
};

} // namespace facebook::react
