#include "TvFeatures.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("missingOnVersion names the Tizen a feature needs on an older TV") {
  CHECK(missingOnVersion(TvFeature::Voice, "5.5") == "Voice control needs Tizen 6.0, and this TV runs 5.5");
  CHECK(
      missingOnVersion(TvFeature::VoiceTitleSelection, "6.0") ==
      "Selecting a title by voice needs Tizen 6.5, and this TV runs 6.0");
  CHECK(missingOnVersion(TvFeature::SmartHubPreview, "2.3").has_value());
}

TEST_CASE("missingOnVersion finds nothing missing from the version on or after a feature") {
  CHECK_FALSE(missingOnVersion(TvFeature::Voice, "6.0").has_value());
  CHECK_FALSE(missingOnVersion(TvFeature::VoiceTitleSelection, "9.0").has_value());
  CHECK_FALSE(missingOnVersion(TvFeature::SmartHubPreview, "6.0").has_value());
}

TEST_CASE("missingOnVersion takes an unreadable version as new enough") {
  CHECK_FALSE(missingOnVersion(TvFeature::Voice, "unknown").has_value());
}

} // namespace rntizen
