#include "WebOSVersion.h"

#include <doctest/doctest.h>

namespace rnwebos {

TEST_CASE("platformVersionNumber gives Platform.Version a number that orders webOS releases") {
  CHECK(platformVersionNumber("10.2.0") == doctest::Approx(10.02));
  CHECK(platformVersionNumber("4.10.0") == doctest::Approx(4.10));
  CHECK(platformVersionNumber("4.5.0") == doctest::Approx(4.05));
  CHECK(platformVersionNumber("1.05.00") == doctest::Approx(1.05));
  CHECK(platformVersionNumber("11.0") == doctest::Approx(11.0));
  CHECK(platformVersionNumber("4.5.0") < platformVersionNumber("4.10.0"));
  CHECK(platformVersionNumber("4.10.0") < platformVersionNumber("5.0.0"));
}

TEST_CASE("platformVersionNumber is 0 for what is not a version") {
  for (const auto *text : {"simulator", "unknown", "9", "9.", "", ".5", "9.123"}) {
    CAPTURE(text);
    CHECK(platformVersionNumber(text) == 0);
  }
}

} // namespace rnwebos
