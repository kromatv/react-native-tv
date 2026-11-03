#include "BlurTint.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <numbers>

namespace rntizen::expo {

namespace {

struct Overlay {
  int gray;
  double alphaPerIntensity;
};

constexpr Overlay kDefault{255, 0.44};
constexpr Overlay kLight{249, 0.78};
constexpr Overlay kDark{25, 0.69};

// expo-blur's Android overlay, TintStyle.toBlurEffect in android/.../enums/TintStyle.kt.
const std::map<std::string, Overlay, std::less<>> &overlays() {
  static const std::map<std::string, Overlay, std::less<>> table = {
      {"default", kDefault},
      {"prominent", kDefault},
      {"systemMaterial", kDefault},
      {"light", kLight},
      {"extraLight", kLight},
      {"systemMaterialLight", kLight},
      {"systemUltraThinMaterialLight", kLight},
      {"systemThickMaterialLight", kLight},
      {"dark", kDark},
      {"systemMaterialDark", kDark},
      {"regular", {179, 0.82}},
      {"systemThinMaterialLight", {199, 0.78}},
      {"systemThinMaterial", {199, 0.97}},
      {"systemChromeMaterial", {255, 0.75}},
      {"systemChromeMaterialLight", {255, 0.97}},
      {"systemUltraThinMaterial", {191, 0.44}},
      {"systemThickMaterial", {153, 0.97}},
      {"systemThickMaterialDark", {37, 0.9}},
      {"systemThinMaterialDark", {37, 0.7}},
      {"systemUltraThinMaterialDark", {37, 0.55}},
      {"systemChromeMaterialDark", {0, 0.75}},
  };
  return table;
}

// Android's Blur::convertRadiusToSigma (libs/hwui/utils/Blur.cpp), which RenderEffect blurs with.
double androidSigma(double radius) {
  return radius > 0 ? std::numbers::inv_sqrt3 * radius + 0.5 : 0;
}

} // namespace

BlurTintColor blurTintColor(const std::string &tint, double intensity) {
  auto found = overlays().find(tint);
  const auto &overlay = found == overlays().end() ? kDefault : found->second;
  const int alpha = std::clamp(static_cast<int>(255 * (intensity / 100) * overlay.alphaPerIntensity), 0, 255);
  const int gray = overlay.gray * alpha / 255;
  return {.red = gray, .green = gray, .blue = gray, .alpha = alpha};
}

int blurRadiusPixels(double intensity, double reductionFactor, double pointScale) {
  const double radius = reductionFactor > 0 ? intensity / reductionFactor : 0;
  return static_cast<int>(std::lround(3 * androidSigma(radius) * pointScale));
}

} // namespace rntizen::expo
