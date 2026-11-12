#pragma once

#include <string>

namespace rntizen {

/// A font's ascent and descent in pixels: the two a textblock line is made of
/// (efl-rendering.md, section 5).
struct FontMetrics {
  int ascent{0};
  int descent{0};
};

/// The metrics of `family` at `weight` (a textblock `font_weight` name), in
/// italic or not, at `pixelSize`, read once from an Evas text object on an
/// offscreen canvas. Main thread only.
FontMetrics fontMetrics(const std::string &family, const std::string &weight, bool italic, int pixelSize);

} // namespace rntizen
