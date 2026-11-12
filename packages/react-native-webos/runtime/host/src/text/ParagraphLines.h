#pragma once

#include <react/renderer/attributedstring/AttributedString.h>
#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/textlayoutmanager/TextMeasureCache.h>

namespace rnwebos {

/// The lines of `text` laid out `width` points wide at `scale` device pixels
/// per point, in points, as `onTextLayout` reports them. A line's `ascender`
/// is how far its baseline is under its own top, so the first line's is the
/// paragraph's baseline, which Yoga aligns on. Any thread.
facebook::react::LinesMeasurements measureParagraphLines(
    const facebook::react::AttributedString &text,
    const facebook::react::ParagraphAttributes &attributes,
    facebook::react::Float width,
    facebook::react::Float scale);

} // namespace rnwebos
