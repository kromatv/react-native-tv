#include "NativeAnimatedModule.h"

#include "../modules/PlatformModuleRegistry.h"
#include "AnimationFrameLoop.h"

#include <react/featureflags/ReactNativeFeatureFlags.h>
#include <react/renderer/animated/AnimatedModule.h>
#include <react/renderer/animated/NativeAnimatedNodesManagerProvider.h>

namespace rntizen {

std::shared_ptr<facebook::react::TurboModule> createNativeAnimatedModule(
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

RN_TIZEN_REGISTRATION(
    (registerPlatformModule(std::string(facebook::react::AnimatedModule::kModuleName), createNativeAnimatedModule),
     true));

} // namespace rntizen
