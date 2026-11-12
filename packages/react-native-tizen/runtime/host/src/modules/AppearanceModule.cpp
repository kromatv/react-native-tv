#include "AppearanceModule.h"

#include "PlatformModuleRegistry.h"

namespace rntizen {

AppearanceModule::AppearanceModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeAppearanceCxxSpec(std::move(jsInvoker)) {}

facebook::jsi::String AppearanceModule::getColorScheme(facebook::jsi::Runtime &runtime) {
  return facebook::jsi::String::createFromUtf8(runtime, colorScheme_);
}

void AppearanceModule::setColorScheme(facebook::jsi::Runtime &runtime, facebook::jsi::String colorScheme) {
  auto requested = colorScheme.utf8(runtime);
  colorScheme_ = requested == "light" ? "light" : "dark";
}

void AppearanceModule::addListener(facebook::jsi::Runtime &, facebook::jsi::String) {}

void AppearanceModule::removeListeners(facebook::jsi::Runtime &, double) {}

RN_TIZEN_PLATFORM_MODULE("Appearance", AppearanceModule);

} // namespace rntizen
