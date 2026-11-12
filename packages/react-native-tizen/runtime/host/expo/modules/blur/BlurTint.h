#pragma once

#include <string>

namespace rntizen::expo {

struct BlurTintColor {
  int red;
  int green;
  int blue;
  int alpha;
};

/// The color expo-blur lays over its blur on Android for `tint` at
/// `intensity` (0 to 100), premultiplied for Evas. An unknown tint is `default`.
BlurTintColor blurTintColor(const std::string &tint, double intensity);

/// How far Evas's blur reaches, in pixels, for expo-blur's `intensity` and
/// `blurReductionFactor`: Android's radius (intensity / factor, in points
/// here), the Gaussian sigma Android blurs that radius with, and the three box
/// passes Evas spans over three sigmas.
int blurRadiusPixels(double intensity, double reductionFactor, double pointScale);

} // namespace rntizen::expo
