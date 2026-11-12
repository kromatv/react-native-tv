#pragma once

#include <react/renderer/core/LayoutConstraints.h>
#include <react/renderer/textlayoutmanager/TextMeasurer.h>

#include "include/core/SkPicture.h"
#include "include/core/SkRefCnt.h"

#include <memory>

namespace rnwebos {

/// A paragraph laid out and recorded for the box it is shown in: what
/// SkiaTextMeasurer prepares on the thread that lays the tree out, and
/// ParagraphView plays. Nothing in it changes, so any thread may draw it.
class PreparedParagraph final : public facebook::react::PreparedText {
 public:
  PreparedParagraph(
      facebook::react::TextMeasurement measurement,
      sk_sp<SkPicture> picture,
      facebook::react::Float scale);

  /// The text in device pixels, its box's top-left corner at the origin.
  const sk_sp<SkPicture> picture;
  /// The device pixels per point the text was laid out at.
  const facebook::react::Float scale;
};

/// Lays `text` out for a box as wide as `constraints` allow, at `scale` device
/// pixels per point, and records what it draws. Any thread.
std::shared_ptr<const PreparedParagraph> prepareParagraph(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    const facebook::react::LayoutConstraints &constraints,
    facebook::react::Float scale);

} // namespace rnwebos
