#include "NativeAnimatedModule.h"

#include "../modules/PlatformModuleRegistry.h"

#include <doctest/doctest.h>
#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/featureflags/ReactNativeFeatureFlagsDefaults.h>
#include <react/renderer/animated/AnimatedModule.h>

#include <memory>

namespace rntizen {

namespace {

using facebook::react::ReactNativeFeatureFlags;

class CxxAnimatedFlags final : public facebook::react::ReactNativeFeatureFlagsDefaults {
 public:
  bool cxxNativeAnimatedEnabled() override {
    return true;
  }
};

class NoInvoker final : public facebook::react::CallInvoker {
 public:
  void invokeAsync(facebook::react::CallFunc &&) noexcept override {}
  void invokeSync(facebook::react::CallFunc &&) override {}
};

} // namespace

TEST_CASE("NativeAnimatedModule is React Native's C++ driver while cxxNativeAnimatedEnabled is on") {
  ReactNativeFeatureFlags::dangerouslyForceOverride(std::make_unique<CxxAnimatedFlags>());

  const auto module = createNativeAnimatedModule(std::make_shared<NoInvoker>(), ModuleContext{});
  const auto registered = findPlatformModule("NativeAnimatedModule");
  ReactNativeFeatureFlags::dangerouslyReset();

  CHECK(std::dynamic_pointer_cast<facebook::react::AnimatedModule>(module) != nullptr);
  CHECK(registered.has_value());
}

TEST_CASE("NativeAnimatedModule is left to JavaScript's driver while cxxNativeAnimatedEnabled is off") {
  ReactNativeFeatureFlags::dangerouslyReset();

  CHECK(createNativeAnimatedModule(std::make_shared<NoInvoker>(), ModuleContext{}) == nullptr);
}

} // namespace rntizen
