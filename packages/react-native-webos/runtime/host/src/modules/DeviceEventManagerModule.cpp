#include "DeviceEventManagerModule.h"

#include "PlatformModuleRegistry.h"

namespace rnwebos {

DeviceEventManagerModule::DeviceEventManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeDeviceEventManagerCxxSpec(std::move(jsInvoker)), exitApp_(context.exitApp) {}

void DeviceEventManagerModule::invokeDefaultBackPressHandler(facebook::jsi::Runtime &) {
  exitApp_();
}

RN_WEBOS_PLATFORM_MODULE("DeviceEventManager", DeviceEventManagerModule);

} // namespace rnwebos
