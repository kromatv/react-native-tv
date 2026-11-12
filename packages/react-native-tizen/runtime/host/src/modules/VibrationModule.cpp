#include "VibrationModule.h"

#include "PlatformModuleRegistry.h"

namespace rntizen {

VibrationModule::VibrationModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &)
    : NativeVibrationCxxSpec(std::move(jsInvoker)) {}

facebook::jsi::Object VibrationModule::getConstants(facebook::jsi::Runtime &runtime) {
  return facebook::jsi::Object(runtime);
}

void VibrationModule::vibrate(facebook::jsi::Runtime &, double) {}

void VibrationModule::vibrateByPattern(facebook::jsi::Runtime &, facebook::jsi::Array, double) {}

void VibrationModule::cancel(facebook::jsi::Runtime &) {}

RN_TIZEN_PLATFORM_MODULE("Vibration", VibrationModule);

} // namespace rntizen
