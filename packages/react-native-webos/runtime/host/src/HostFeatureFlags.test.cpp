#include "HostFeatureFlags.h"

#include <doctest/doctest.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>

namespace rnwebos {

TEST_CASE("overrideFeatureFlagsOnce turns on the OSS Stable flags and the C++ animated driver, once per process") {
  using facebook::react::ReactNativeFeatureFlags;
  ReactNativeFeatureFlags::dangerouslyReset();

  overrideFeatureFlagsOnce();
  CHECK_NOTHROW(overrideFeatureFlagsOnce());

  CHECK(ReactNativeFeatureFlags::cxxNativeAnimatedEnabled());
  CHECK(ReactNativeFeatureFlags::useNativeViewConfigsInBridgelessMode());
  CHECK(ReactNativeFeatureFlags::enablePreparedTextLayout());
  ReactNativeFeatureFlags::dangerouslyReset();
}

} // namespace rnwebos
