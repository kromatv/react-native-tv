#include "StatusBarManagerModule.h"

#include "PlatformModuleRegistry.h"

namespace rnwebos {

StatusBarManagerModule::StatusBarManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &)
    : NativeStatusBarManagerAndroidCxxSpec(std::move(jsInvoker)) {}

facebook::jsi::Object StatusBarManagerModule::getConstants(facebook::jsi::Runtime &runtime) {
  facebook::jsi::Object constants(runtime);
  constants.setProperty(runtime, "HEIGHT", 0);
  constants.setProperty(runtime, "DEFAULT_BACKGROUND_COLOR", 0);
  return constants;
}

void StatusBarManagerModule::setStyle(facebook::jsi::Runtime &, std::optional<facebook::jsi::String>) {}

void StatusBarManagerModule::setHidden(facebook::jsi::Runtime &, bool) {}

RN_WEBOS_PLATFORM_MODULE("StatusBarManager", StatusBarManagerModule);

} // namespace rnwebos
