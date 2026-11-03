#pragma once

#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/attributedstring/TextAttributes.h>

#include <optional>
#include <string>

namespace rntizen {

/// The textblock format of one run of text: font, size in pixels, weight,
/// slant, color, alignment, line height and decorations (efl-rendering.md
/// section 5). `scale` turns points into pixels. Main thread only: a line
/// height below the font's own reads the font's metrics.
std::string fragmentFormat(const facebook::react::TextAttributes &attributes, facebook::react::Float scale);

/// For a run whose `lineHeight` is below its font's own line, which Evas then
/// shrinks in proportion to ascent and descent: how many pixels lower CSS puts
/// its baseline, splitting the shortfall evenly above and below the glyphs.
/// Nothing for any other run. Main thread only, like `fragmentFormat`.
std::optional<int> tightLineShift(const facebook::react::TextAttributes &attributes, facebook::react::Float scale);

/// The paragraph's default format: the fallback font, then wrapping and, for a
/// single line, the ellipsis from `ellipsizeMode`, and the alignment. More
/// lines are clamped by `clampLines`.
std::string paragraphFormat(
    const facebook::react::ParagraphAttributes &paragraph,
    std::optional<facebook::react::TextAlignment> alignment,
    facebook::react::Float scale);

} // namespace rntizen
