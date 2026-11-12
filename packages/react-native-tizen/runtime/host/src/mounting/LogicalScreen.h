#pragma once

#include <react/renderer/graphics/Size.h>

namespace rntizen {

/// The screen every app lays out for, in points, whatever window the TV gives
/// it (docs/decisions.md, "One logical 1920x1080 screen").
inline constexpr facebook::react::Size kLogicalScreenSize{.width = 1920, .height = 1080};

/// Device pixels per point in a window of `windowSize` pixels: the largest
/// scale at which the logical screen fits, 2/3 in a 1280x720 window.
facebook::react::Float pointScaleFactor(facebook::react::Size windowSize);

} // namespace rntizen
