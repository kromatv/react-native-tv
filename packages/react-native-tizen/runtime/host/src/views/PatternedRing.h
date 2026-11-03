#pragma once

#include "NinePatch.h"

namespace rntizen {

/// How a border is stroked: CSS `border-style` as React Native has it.
enum class StrokePattern { Solid, Dashed, Dotted };

/// A `width` x `height` border ring with `corners` and per-side `widths`, cut
/// into dashes or dots along its centerline, corners included, sized and
/// spaced as Chromium paints CSS borders (efl-rendering.md, section 4): a
/// white patch the size of the box, with no stretchable border. Null when the
/// box passes 2048 pixels on a side, or its outline is too short for two
/// dashes: the caller then draws a solid ring. Cached; a returned patch stays
/// valid until the next call. Main thread only.
const NinePatch *patternedRingPatch(
    const Corners &corners,
    const Edges &widths,
    int width,
    int height,
    StrokePattern pattern);

} // namespace rntizen
