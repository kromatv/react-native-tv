#include "DeviceEventManagerModule.h"

#include "PlatformModuleRegistry.h"

namespace rntizen {

DeviceEventManagerModule::DeviceEventManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeDeviceEventManagerCxxSpec(std::move(jsInvoker)), exitApp_(context.exitApp) {}

void DeviceEventManagerModule::invokeDefaultBackPressHandler(facebook::jsi::Runtime &) {
  exitApp_();
}

RN_TIZEN_PLATFORM_MODULE("DeviceEventManager", DeviceEventManagerModule);

} // namespace rntizen
