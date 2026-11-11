#include "BlurTint.h"

#include <doctest/doctest.h>

namespace rntizen::expo {

namespace {

bool same(const BlurTintColor &color, int gray, int alpha) {
  return color.red == gray && color.green == gray && color.blue == gray && color.alpha == alpha;
}

} // namespace

TEST_CASE("blurTintColor lays Android's overlay for the tint over the blur, premultiplied") {
  CHECK(same(blurTintColor("default", 100), 112, 112));
  CHECK(same(blurTintColor("dark", 100), 17, 175));
  CHECK(same(blurTintColor("light", 50), 96, 99));
  CHECK(same(blurTintColor("regular", 100), 146, 209));
  CHECK(same(blurTintColor("systemChromeMaterialDark", 100), 0, 191));
  CHECK(same(blurTintColor("systemUltraThinMaterialDark", 80), 16, 112));
  CHECK(same(blurTintColor("extraLight", 50), 96, 99));
  CHECK(same(blurTintColor("systemMaterialDark", 100), 17, 175));
}

TEST_CASE("blurTintColor treats an unknown tint as default and keeps alpha within a byte") {
  CHECK(same(blurTintColor("neon", 100), 112, 112));
  CHECK(same(blurTintColor("default", 0), 0, 0));
  CHECK(same(blurTintColor("default", -10), 0, 0));
  CHECK(same(blurTintColor("default", 1000), 255, 255));
}

TEST_CASE("blurRadiusPixels spans three of Android's sigmas for intensity over the reduction factor") {
  CHECK(blurRadiusPixels(50, 4, 1) == 23);
  CHECK(blurRadiusPixels(50, 4, 2) == 46);
  CHECK(blurRadiusPixels(100, 4, 1.5) == 67);
  CHECK(blurRadiusPixels(0, 4, 1) == 0);
  CHECK(blurRadiusPixels(50, 0, 1) == 0);
}

} // namespace rntizen::expo
