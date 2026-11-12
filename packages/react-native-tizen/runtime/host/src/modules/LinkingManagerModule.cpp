#include "LinkingManagerModule.h"

#include "PlatformModuleRegistry.h"
#include "Promises.h"

namespace rntizen {

LinkingManagerModule::LinkingManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeLinkingManagerCxxSpec(std::move(jsInvoker)), launchUrl_(context.launchUrl) {}

facebook::jsi::Value LinkingManagerModule::getInitialURL(facebook::jsi::Runtime &runtime) {
  auto url = launchUrl_ != nullptr ? launchUrl_->get() : std::nullopt;
  if (!url) return resolvedPromise(runtime, facebook::jsi::Value::null());
  return resolvedPromise(runtime, facebook::jsi::String::createFromUtf8(runtime, *url));
}

facebook::jsi::Value LinkingManagerModule::canOpenURL(facebook::jsi::Runtime &runtime, facebook::jsi::String) {
  return resolvedPromise(runtime, facebook::jsi::Value(false));
}

facebook::jsi::Value LinkingManagerModule::openURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url) {
  return rejectedPromise(
      runtime, "Cannot open " + url.utf8(runtime) + ": Linking.openURL is not supported on Tizen yet");
}

facebook::jsi::Value LinkingManagerModule::openSettings(facebook::jsi::Runtime &runtime) {
  return rejectedPromise(runtime, "Linking.openSettings is not supported on Tizen");
}

void LinkingManagerModule::addListener(facebook::jsi::Runtime &, facebook::jsi::String) {}

void LinkingManagerModule::removeListeners(facebook::jsi::Runtime &, double) {}

RN_TIZEN_PLATFORM_MODULE("LinkingManager", LinkingManagerModule);

} // namespace rntizen
