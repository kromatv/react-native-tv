#pragma once

#include <react/renderer/graphics/Color.h>

#include "include/core/SkColor.h"

namespace rnwebos {

/// `color` as Skia takes it, its alpha multiplied by `opacity`. An unset color is transparent.
SkColor4f skColor(const facebook::react::SharedColor &color, float opacity = 1);

/// Whether `color` is set and not fully transparent.
bool isVisible(const facebook::react::SharedColor &color);

/// Whether `color` hides everything under it.
bool isOpaque(const facebook::react::SharedColor &color);

} // namespace rnwebos
