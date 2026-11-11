#pragma once

#include "include/core/SkColor.h"

#include <string>

namespace rnwebos::expo {

/// The color expo-blur lays over its blur on Android for `tint` at
/// `intensity` (0 to 100). An unknown tint is `default`.
SkColor blurTintColor(const std::string &tint, double intensity);

/// The Gaussian deviation of the blur, in pixels, for expo-blur's `intensity`
/// and `blurReductionFactor`: Android's radius (intensity / factor, in points
/// here) and the sigma Android blurs that radius with.
float blurSigmaPixels(double intensity, double reductionFactor, double pointScale);

} // namespace rnwebos::expo
