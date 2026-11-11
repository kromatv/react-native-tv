#include "HostFeatureFlags.h"

#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/featureflags/ReactNativeFeatureFlagsOverridesOSSStable.h>

#include <memory>
#include <mutex>

namespace rnwebos {

namespace {

class WebOSFeatureFlags final : public facebook::react::ReactNativeFeatureFlagsOverridesOSSStable {
 public:
  bool cxxNativeAnimatedEnabled() override {
    return true;
  }
};

} // namespace

void overrideFeatureFlagsOnce() {
  static std::once_flag once;
  std::call_once(
      once, [] { facebook::react::ReactNativeFeatureFlags::override(std::make_unique<WebOSFeatureFlags>()); });
}

} // namespace rnwebos
