#include "TizenVersion.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("parseTizenVersion reads the major and minor of what system_info reports") {
  CHECK(parseTizenVersion("6.5") == TizenVersion{.major = 6, .minor = 5});
  CHECK(parseTizenVersion("10.0.0") == TizenVersion{.major = 10, .minor = 0});
  CHECK_FALSE(parseTizenVersion("unknown").has_value());
  CHECK_FALSE(parseTizenVersion("9").has_value());
  CHECK_FALSE(parseTizenVersion("9.").has_value());
  CHECK_FALSE(parseTizenVersion("").has_value());
}

TEST_CASE("Tizen versions order by major, then minor") {
  CHECK(TizenVersion{.major = 6, .minor = 5} < TizenVersion{.major = 10, .minor = 0});
  CHECK(TizenVersion{.major = 6, .minor = 0} < TizenVersion{.major = 6, .minor = 5});
}

TEST_CASE("platformVersionNumber gives Platform.Version a comparable number") {
  CHECK(platformVersionNumber("6.5") == doctest::Approx(6.5));
  CHECK(platformVersionNumber("9.0") == doctest::Approx(9.0));
  CHECK(platformVersionNumber("10.0.0") == doctest::Approx(10.0));
  CHECK(platformVersionNumber("unknown") == doctest::Approx(0.0));
}

} // namespace rntizen
