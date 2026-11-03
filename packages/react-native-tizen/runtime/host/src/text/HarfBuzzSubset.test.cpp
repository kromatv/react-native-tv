#include "HarfBuzzSubset.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("harfBuzzSubset resolves every call or none") {
  const auto *api = harfBuzzSubset();

  if (api != nullptr) {
    CHECK(api->blobCreateFromFile != nullptr);
    CHECK(api->subset != nullptr);
  }
  CHECK(harfBuzzSubset() == api);
}

TEST_CASE("hbTag packs four characters big-endian, as HB_TAG does") {
  CHECK(hbTag('w', 'g', 'h', 't') == 0x77676874U);
}

} // namespace rntizen
