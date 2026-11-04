#include "HarfBuzzSubset.h"

#include <doctest/doctest.h>

namespace rntizen {

namespace {

constexpr const char *kCore = "libharfbuzz.so.0";
constexpr const char *kSubset = "libharfbuzz-subset.so.0";
constexpr const char *kMissing = "librn-tizen-missing.so.0";

} // namespace

TEST_CASE("harfBuzzSubset resolves every call or none") {
  const auto *api = harfBuzzSubset();

  if (api != nullptr) {
    CHECK(api->blobCreateFromFile != nullptr);
    CHECK(api->subset != nullptr);
  }
  CHECK(harfBuzzSubset() == api);
}

TEST_CASE("loadHarfBuzzSubset has nothing when either library is missing") {
  CHECK_FALSE(loadHarfBuzzSubset(kMissing, kSubset).has_value());
  CHECK_FALSE(loadHarfBuzzSubset(kCore, kMissing).has_value());
}

TEST_CASE("loadHarfBuzzSubset has nothing from a library that lacks the subsetter's calls") {
  CHECK_FALSE(loadHarfBuzzSubset(kCore, kCore).has_value());
}

TEST_CASE("hbTag packs four characters big-endian, as HB_TAG does") {
  CHECK(hbTag('w', 'g', 'h', 't') == 0x77676874U);
}

} // namespace rntizen
