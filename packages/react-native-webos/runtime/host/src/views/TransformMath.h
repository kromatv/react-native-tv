#pragma once

#include <react/renderer/graphics/Float.h>
#include <react/renderer/graphics/Size.h>
#include <react/renderer/graphics/Transform.h>

namespace rnwebos {

/// Whether `transform` moves points without turning, scaling or skewing them.
bool isTranslationOnly(const facebook::react::Transform &transform);

/// `transform` about the center of a `size` box rather than its top-left
/// corner: React Native's default `transformOrigin`.
facebook::react::Transform aroundCenter(const facebook::react::Transform &transform, const facebook::react::Size &size);

} // namespace rnwebos
