#pragma once

#include <react/renderer/attributedstring/ParagraphAttributes.h>
#include <react/renderer/attributedstring/TextAttributes.h>

#include <optional>
#include <string>

namespace rntizen {

/// How one run of text is drawn.
struct FragmentFormat {
  /// The textblock format: font, size in pixels, weight, slant, color,
  /// alignment, line height and decorations (efl-rendering.md section 5).
  std::string format;
  /// For a run whose `lineHeight` is below its font's own line, which Evas
  /// then shrinks in proportion to ascent and descent: how many pixels lower
  /// CSS puts its baseline, splitting the shortfall evenly above and below the
  /// glyphs. Nothing for any other run.
  std::optional<int> tightLineShift;
};

/// The format of a run with `attributes`. `scale` turns points into pixels.
/// Main thread only: a line height below the font's own reads the font's
/// metrics.
FragmentFormat fragmentFormat(const facebook::react::TextAttributes &attributes, facebook::react::Float scale);

/// The paragraph's default format: the fallback font, then wrapping and, for a
/// single line, the ellipsis from `ellipsizeMode`, and the alignment. More
/// lines are clamped by `clampLines`.
std::string paragraphFormat(
    const facebook::react::ParagraphAttributes &paragraph,
    std::optional<facebook::react::TextAlignment> alignment,
    facebook::react::Float scale);

} // namespace rntizen
