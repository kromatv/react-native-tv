#include "AnimationFrameLoop.h"

#include "../modules/PlatformModuleRegistry.h"

#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/animated/AnimatedModule.h>
#include <react/renderer/animated/NativeAnimatedNodesManagerProvider.h>

namespace rntizen {

namespace {

std::shared_ptr<facebook::react::TurboModule> createAnimatedModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &) {
  using namespace facebook::react;
  if (!ReactNativeFeatureFlags::cxxNativeAnimatedEnabled()) return nullptr;
  auto frames = std::make_shared<AnimationFrameLoop>();
  auto provider = std::make_shared<NativeAnimatedNodesManagerProvider>(
      [frames](std::function<void()> &&onRender, bool) { frames->start(std::move(onRender)); },
      [frames](bool) { frames->stop(); });
  frames->follow(provider);
  return std::make_shared<AnimatedModule>(std::move(jsInvoker), std::move(provider));
}

} // namespace

RN_TIZEN_REGISTRATION(
    (registerPlatformModule(std::string(facebook::react::AnimatedModule::kModuleName), createAnimatedModule), true));

} // namespace rntizen
