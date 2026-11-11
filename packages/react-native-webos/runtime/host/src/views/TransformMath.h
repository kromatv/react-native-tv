#pragma once

#include "PixelGeometry.h"

#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/Size.h>
#include <react/renderer/graphics/Transform.h>

namespace rntizen {

/// Whether `transform` moves points without turning, scaling or skewing them.
bool isTranslationOnly(const facebook::react::Transform &transform);

/// `transform` about the center of a `size` box rather than its top-left
/// corner: React Native's default `transformOrigin`.
facebook::react::Transform aroundCenter(const facebook::react::Transform &transform, const facebook::react::Size &size);

/// Where `point`, in window pixels, shows once `transform` applies to a view
/// whose untransformed box is `box`, at `scale` pixels per point.
PixelPoint transformedPoint(
    const facebook::react::Transform &transform,
    PixelPoint point,
    const PixelRect &box,
    facebook::react::Float scale);

} // namespace rntizen
