#include "LogBoxModule.h"

#include "DevSupport.h"

#include "modules/PlatformModuleRegistry.h"

namespace rnwebos {

LogBoxModule::LogBoxModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeLogBoxCxxSpec(std::move(jsInvoker)) {}

void LogBoxModule::show(facebook::jsi::Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.setLogBoxShown(true); });
}

void LogBoxModule::hide(facebook::jsi::Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.setLogBoxShown(false); });
}

RN_WEBOS_PLATFORM_MODULE("LogBox", LogBoxModule);

} // namespace rnwebos
