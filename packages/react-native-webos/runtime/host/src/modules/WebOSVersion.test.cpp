#include "WebOSVersion.h"

#include <doctest/doctest.h>

namespace rnwebos {

TEST_CASE("parseWebOSVersion reads the major and minor of what system_info reports") {
  CHECK(parseWebOSVersion("6.5") == WebOSVersion{.major = 6, .minor = 5});
  CHECK(parseWebOSVersion("10.0.0") == WebOSVersion{.major = 10, .minor = 0});
  CHECK_FALSE(parseWebOSVersion("unknown").has_value());
  CHECK_FALSE(parseWebOSVersion("9").has_value());
  CHECK_FALSE(parseWebOSVersion("9.").has_value());
  CHECK_FALSE(parseWebOSVersion("").has_value());
}

TEST_CASE("webOS versions order by major, then minor") {
  CHECK(WebOSVersion{.major = 6, .minor = 5} < WebOSVersion{.major = 10, .minor = 0});
  CHECK(WebOSVersion{.major = 6, .minor = 0} < WebOSVersion{.major = 6, .minor = 5});
}

TEST_CASE("platformVersionNumber gives Platform.Version a comparable number") {
  CHECK(platformVersionNumber("6.5") == doctest::Approx(6.5));
  CHECK(platformVersionNumber("9.0") == doctest::Approx(9.0));
  CHECK(platformVersionNumber("10.0.0") == doctest::Approx(10.0));
  CHECK(platformVersionNumber("unknown") == doctest::Approx(0.0));
}

} // namespace rnwebos
