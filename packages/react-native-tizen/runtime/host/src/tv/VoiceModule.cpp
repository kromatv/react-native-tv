#include "../modules/PlatformModuleRegistry.h"
#include "TvState.h"

#include <ReactCommon/TurboModule.h>

namespace rntizen {

namespace {

using facebook::jsi::Runtime;
using facebook::jsi::Value;
using facebook::react::TurboModule;

class VoiceModule final : public TurboModule {
 public:
  VoiceModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
      : TurboModule("RNTizenVoice", std::move(jsInvoker)), state_(context.tv) {
    methodMap_["isAvailable"] = MethodMetadata{.argCount = 0, .invoker = &isAvailable};
    methodMap_["setScreen"] = MethodMetadata{.argCount = 1, .invoker = &setScreen};
    methodMap_["addListener"] = MethodMetadata{.argCount = 1, .invoker = &addListener};
    methodMap_["removeListener"] = MethodMetadata{.argCount = 1, .invoker = &removeListener};
  }

 private:
  static TvState *state(TurboModule &module) {
    return static_cast<VoiceModule &>(module).state_.get();
  }

  static Value isAvailable(Runtime &, TurboModule &module, const Value *, size_t) {
    auto *tv = state(module);
    return Value(tv != nullptr && tv->voiceAvailable());
  }

  static Value setScreen(Runtime &runtime, TurboModule &module, const Value *arguments, size_t count) {
    auto *tv = state(module);
    if (tv == nullptr) return Value::undefined();
    if (count == 0 || !arguments[0].isString()) {
      tv->setScreen(std::nullopt);
    } else {
      tv->setScreen(arguments[0].getString(runtime).utf8(runtime));
    }
    return Value::undefined();
  }

  static Value changeListeners(Runtime &runtime, TurboModule &module, const Value *arguments, size_t count, int delta) {
    auto *tv = state(module);
    if (tv != nullptr && count > 0 && arguments[0].isString()) {
      tv->addListener(arguments[0].getString(runtime).utf8(runtime), delta);
    }
    return Value::undefined();
  }

  static Value addListener(Runtime &runtime, TurboModule &module, const Value *arguments, size_t count) {
    return changeListeners(runtime, module, arguments, count, 1);
  }

  static Value removeListener(Runtime &runtime, TurboModule &module, const Value *arguments, size_t count) {
    return changeListeners(runtime, module, arguments, count, -1);
  }

  const std::shared_ptr<TvState> state_;
};

} // namespace

RN_TIZEN_PLATFORM_MODULE("RNTizenVoice", VoiceModule);

} // namespace rntizen
