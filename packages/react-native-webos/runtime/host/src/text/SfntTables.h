#pragma once

#include <optional>
#include <string>

namespace rntizen {

/// What fontconfig would match a font file on: its `OS/2` weight class and
/// whether it is italic.
struct SfntStyle {
  int weight;
  bool italic;
};

/// The style of a TrueType or OpenType font held in `bytes`, or nothing for a
/// collection or a file without an `OS/2` table.
std::optional<SfntStyle> sfntStyle(const std::string &bytes);

/// Adds `units` (font units, negative to tighten) to every advance width in the
/// `hmtx` table of the font in `bytes`, never below zero. The file keeps its
/// size and table offsets; FreeType does not check table checksums. Returns
/// false when the font has no usable `head`, `hhea` and `hmtx`.
bool trackSfntAdvances(std::string &bytes, int units);

/// The font's units per em from its `head` table, or nothing.
std::optional<int> sfntUnitsPerEm(const std::string &bytes);

} // namespace rntizen
