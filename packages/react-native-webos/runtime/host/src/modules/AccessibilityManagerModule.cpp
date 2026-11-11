#include "AccessibilityManagerModule.h"

#include "PlatformModuleRegistry.h"

#include <utility>

namespace rnwebos {

namespace {

constexpr const char *kGetSystemSettings = "luna://com.webos.settingsservice/getSystemSettings";

folly::dynamic audioGuidanceRequest(bool subscribe) {
  return folly::dynamic::object("category", "option")("keys", folly::dynamic::array("audioGuidance"))(
      "subscribe", subscribe);
}

void answerOff(facebook::jsi::Runtime &runtime, const facebook::jsi::Function &onSuccess) {
  onSuccess.call(runtime, false);
}

} // namespace

std::optional<bool> audioGuidanceOf(const folly::dynamic &reply) {
  const auto *settings = reply.isObject() ? reply.get_ptr("settings") : nullptr;
  const auto *guidance = settings != nullptr && settings->isObject() ? settings->get_ptr("audioGuidance") : nullptr;
  if (guidance == nullptr || !guidance->isString()) return std::nullopt;
  return guidance->getString() == "on";
}

AccessibilityManagerModule::AccessibilityManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeAccessibilityManagerCxxSpec(std::move(jsInvoker)), luna_(context.luna) {
  if (luna_ == nullptr || !context.emitDeviceEvent) return;
  // The first reply is the setting as it is: only a later, different one is a change.
  auto last = std::make_shared<std::optional<bool>>();
  audioGuidance_ = luna_->subscribe(
      kGetSystemSettings, audioGuidanceRequest(true), [last, emit = context.emitDeviceEvent](const folly::dynamic &reply) {
        const auto enabled = audioGuidanceOf(reply);
        if (!enabled) return;
        const auto previous = std::exchange(*last, enabled);
        if (previous.has_value() && previous != enabled) emit("screenReaderChanged", *enabled);
      });
}

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
    facebook::jsi::Runtime &,
    facebook::react::AsyncCallback<bool> onSuccess,
    Callback) {
  const bool asked = luna_ != nullptr &&
      luna_->call(kGetSystemSettings, audioGuidanceRequest(false), [onSuccess](const folly::dynamic &reply) {
        onSuccess.call(audioGuidanceOf(reply).value_or(false));
      });
  if (!asked) onSuccess.call(false);
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

RN_WEBOS_PLATFORM_MODULE("AccessibilityManager", AccessibilityManagerModule);

} // namespace rnwebos
