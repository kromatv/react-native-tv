#include "BlurTint.h"

#include <doctest/doctest.h>

namespace rnwebos::expo {

namespace {

bool same(SkColor color, unsigned gray, unsigned alpha) {
  return color == SkColorSetARGB(alpha, gray, gray, gray);
}

} // namespace

TEST_CASE("blurTintColor lays Android's overlay for the tint over the blur") {
  CHECK(same(blurTintColor("default", 100), 255, 112));
  CHECK(same(blurTintColor("dark", 100), 25, 175));
  CHECK(same(blurTintColor("light", 50), 249, 99));
  CHECK(same(blurTintColor("regular", 100), 179, 209));
  CHECK(same(blurTintColor("systemChromeMaterialDark", 100), 0, 191));
  CHECK(same(blurTintColor("systemUltraThinMaterialDark", 80), 37, 112));
  CHECK(same(blurTintColor("extraLight", 50), 249, 99));
  CHECK(same(blurTintColor("systemMaterialDark", 100), 25, 175));
}

TEST_CASE("blurTintColor treats an unknown tint as default and keeps alpha within a byte") {
  CHECK(same(blurTintColor("neon", 100), 255, 112));
  CHECK(same(blurTintColor("default", 0), 255, 0));
  CHECK(same(blurTintColor("default", -10), 255, 0));
  CHECK(same(blurTintColor("default", 1000), 255, 255));
}

TEST_CASE("blurSigmaPixels is Android's sigma for intensity over the reduction factor, in pixels") {
  CHECK(blurSigmaPixels(50, 4, 1) == doctest::Approx(7.7169));
  CHECK(blurSigmaPixels(50, 4, 2) == doctest::Approx(15.4338));
  CHECK(blurSigmaPixels(100, 4, 1.5) == doctest::Approx(22.4006));
  CHECK(blurSigmaPixels(0, 4, 1) == 0);
  CHECK(blurSigmaPixels(50, 0, 1) == 0);
}

} // namespace rnwebos::expo
