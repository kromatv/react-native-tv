#pragma once

#include <react/renderer/attributedstring/TextAttributes.h>

#include "modules/skparagraph/include/TextStyle.h"

namespace rnwebos {

/// How SkParagraph draws a run with `attributes`: font, size in pixels at
/// `scale` pixels per point, weight, slant, colors, spacing, line height,
/// decorations and shadow. A `lineHeight` sets the line's height whatever the
/// font's own, the difference shared above and below the glyphs as CSS does.
skia::textlayout::TextStyle textStyle(const facebook::react::TextAttributes &attributes, facebook::react::Float scale);

} // namespace rnwebos
