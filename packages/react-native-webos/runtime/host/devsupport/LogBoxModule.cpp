#include "LogBoxModule.h"

#include "DevSupport.h"

#include "modules/PlatformModuleRegistry.h"

namespace rntizen {

LogBoxModule::LogBoxModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeLogBoxCxxSpec(std::move(jsInvoker)) {}

void LogBoxModule::show(facebook::jsi::Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.setLogBoxShown(true); });
}

void LogBoxModule::hide(facebook::jsi::Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.setLogBoxShown(false); });
}

RN_TIZEN_PLATFORM_MODULE("LogBox", LogBoxModule);

} // namespace rntizen
