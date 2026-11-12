#pragma once

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/graphics/Rect.h>

#include "modules/skparagraph/include/Paragraph.h"

#include <memory>
#include <vector>

namespace rnwebos {

/// A paragraph shaped and broken into lines by SkParagraph, the engine that
/// also draws it, so the size Yoga gets is the size of the pixels.
struct ParagraphLayout {
  /// Laid out in device pixels; paint it at the content box's top-left corner.
  std::unique_ptr<skia::textlayout::Paragraph> paragraph;
  /// The text's size in points: its widest line and its lines' height, each
  /// rounded up to a whole pixel.
  facebook::react::Size size;
  /// Where each inline view sits in the text, in points, in fragment order.
  std::vector<facebook::react::Rect> attachments;
};

/// Lays `text` out in lines at most `maximumWidth` points wide (infinite for
/// one unbroken line per paragraph), at `scale` device pixels per point.
/// Any thread: it takes the text engine's lock.
ParagraphLayout layOutParagraph(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    facebook::react::Float maximumWidth,
    facebook::react::Float scale);

/// Lays `text` out as `layOutParagraph` does, then, when its lines are not
/// left-aligned, in the box it will be shown in: as wide as its widest line,
/// at least `minimumWidth`. Centered text then sits where that box puts it.
ParagraphLayout layOutParagraphInBox(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    facebook::react::Float minimumWidth,
    facebook::react::Float maximumWidth,
    facebook::react::Float scale);

} // namespace rnwebos
