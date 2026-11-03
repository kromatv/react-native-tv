#include "DevSettingsModule.h"

#include "DevSupport.h"

#include "modules/PlatformModuleRegistry.h"

namespace rntizen {

using facebook::jsi::Runtime;

DevSettingsModule::DevSettingsModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeDevSettingsCxxSpec(std::move(jsInvoker)) {}

void DevSettingsModule::reload(Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.reload("DevSettings.reload()"); });
}

void DevSettingsModule::reloadWithReason(Runtime &, std::string reason) {
  DevSupport::post([reason = std::move(reason)](DevSupport &devSupport) { devSupport.reload(reason); });
}

void DevSettingsModule::onFastRefresh(Runtime &) {}

void DevSettingsModule::setHotLoadingEnabled(Runtime &, bool enabled) {
  DevSupport::post([enabled](DevSupport &devSupport) { devSupport.setHotLoadingEnabled(enabled); });
}

void DevSettingsModule::setProfilingEnabled(Runtime &, bool) {}

void DevSettingsModule::toggleElementInspector(Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.toggleElementInspector(); });
}

void DevSettingsModule::addMenuItem(Runtime &, const std::string &) {}

void DevSettingsModule::openDebugger(Runtime &) {}

void DevSettingsModule::addListener(Runtime &, const std::string &) {}

void DevSettingsModule::removeListeners(Runtime &, double) {}

void DevSettingsModule::setIsShakeToShowDevMenuEnabled(Runtime &, bool) {}

RN_TIZEN_PLATFORM_MODULE("DevSettings", DevSettingsModule);

} // namespace rntizen
