#include "VoiceModule.h"

#include "../modules/PlatformModuleRegistry.h"
#include "TvState.h"

namespace rntizen {

using facebook::jsi::Runtime;
using facebook::jsi::Value;
using facebook::react::TurboModule;

VoiceModule::VoiceModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : TurboModule("RNTizenVoice", std::move(jsInvoker)), state_(context.tv) {
  methodMap_["isAvailable"] = MethodMetadata{.argCount = 0, .invoker = &isAvailable};
  methodMap_["setScreen"] = MethodMetadata{.argCount = 1, .invoker = &setScreen};
  methodMap_["addListener"] = MethodMetadata{.argCount = 1, .invoker = &addListener};
  methodMap_["removeListener"] = MethodMetadata{.argCount = 1, .invoker = &removeListener};
}

TvState *VoiceModule::state(const TurboModule &turboModule) {
  return static_cast<const VoiceModule &>(turboModule).state_.get();
}

Value VoiceModule::isAvailable(Runtime &, TurboModule &turboModule, const Value *, size_t) {
  const auto *tv = state(turboModule);
  return Value(tv != nullptr && tv->voiceAvailable());
}

Value VoiceModule::setScreen(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count) {
  auto *tv = state(turboModule);
  if (tv == nullptr) return Value::undefined();
  if (count == 0 || !arguments[0].isString()) {
    tv->setScreen(std::nullopt);
  } else {
    tv->setScreen(arguments[0].getString(runtime).utf8(runtime));
  }
  return Value::undefined();
}

Value VoiceModule::changeListeners(
    Runtime &runtime, const TurboModule &turboModule, const Value *arguments, size_t count, int delta) {
  if (auto *tv = state(turboModule); tv != nullptr && count > 0 && arguments[0].isString()) {
    tv->addListener(arguments[0].getString(runtime).utf8(runtime), delta);
  }
  return Value::undefined();
}

Value VoiceModule::addListener(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count) {
  return changeListeners(runtime, turboModule, arguments, count, 1);
}

Value VoiceModule::removeListener(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count) {
  return changeListeners(runtime, turboModule, arguments, count, -1);
}

RN_TIZEN_PLATFORM_MODULE("RNTizenVoice", VoiceModule);

} // namespace rntizen
