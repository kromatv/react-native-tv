#include "AppStateModule.h"

#include "PlatformModuleRegistry.h"

namespace rntizen {

AppStateModule::AppStateModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : NativeAppStateCxxSpec(std::move(jsInvoker)), isActive_(context.isActive) {}

const char *AppStateModule::stateName(bool isActive) {
  return isActive ? "active" : "background";
}

facebook::jsi::Object AppStateModule::getConstants(facebook::jsi::Runtime &runtime) {
  facebook::jsi::Object constants(runtime);
  constants.setProperty(runtime, "initialAppState", stateName(isActive_->load()));
  return constants;
}

void AppStateModule::getCurrentAppState(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::Function success,
    facebook::jsi::Function) {
  facebook::jsi::Object state(runtime);
  state.setProperty(runtime, "app_state", stateName(isActive_->load()));
  success.call(runtime, state);
}

void AppStateModule::addListener(facebook::jsi::Runtime &, facebook::jsi::String) {}

void AppStateModule::removeListeners(facebook::jsi::Runtime &, double) {}

RN_TIZEN_PLATFORM_MODULE("AppState", AppStateModule);

} // namespace rntizen
