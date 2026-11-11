#include "AccessibilityManagerModule.h"

#include "PlatformModuleRegistry.h"

namespace rntizen {

namespace {

void answerOff(facebook::jsi::Runtime &runtime, const facebook::jsi::Function &onSuccess) {
  onSuccess.call(runtime, false);
}

} // namespace

AccessibilityManagerModule::AccessibilityManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &)
    : NativeAccessibilityManagerCxxSpec(std::move(jsInvoker)) {}

void AccessibilityManagerModule::getCurrentBoldTextState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentGrayscaleState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentInvertColorsState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentReduceMotionState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentDarkerSystemColorsState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentPrefersCrossFadeTransitionsState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentReduceTransparencyState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::getCurrentVoiceOverState(
    facebook::jsi::Runtime &runtime,
    Callback onSuccess,
    Callback) {
  answerOff(runtime, onSuccess);
}

void AccessibilityManagerModule::setAccessibilityContentSizeMultipliers(
    facebook::jsi::Runtime &,
    facebook::jsi::Object) {}

void AccessibilityManagerModule::setAccessibilityFocus(facebook::jsi::Runtime &, double) {}

void AccessibilityManagerModule::announceForAccessibility(facebook::jsi::Runtime &, facebook::jsi::String) {}

void AccessibilityManagerModule::announceForAccessibilityWithOptions(
    facebook::jsi::Runtime &,
    facebook::jsi::String,
    facebook::jsi::Object) {}

RN_TIZEN_PLATFORM_MODULE("AccessibilityManager", AccessibilityManagerModule);

} // namespace rntizen
