#include "MicButtonModule.h"

#include "../input/MicButton.h"
#include "../modules/PlatformModuleRegistry.h"

namespace rntizen {

using facebook::jsi::Runtime;
using facebook::jsi::Value;
using facebook::react::TurboModule;

MicButtonModule::MicButtonModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context)
    : TurboModule("RNTizenMicButton", std::move(jsInvoker)), button_(context.micButton) {
  methodMap_["isAvailable"] = MethodMetadata{.argCount = 0, .invoker = &isAvailable};
  methodMap_["addListener"] = MethodMetadata{.argCount = 1, .invoker = &addListener};
  methodMap_["removeListener"] = MethodMetadata{.argCount = 1, .invoker = &removeListener};
}

Value MicButtonModule::isAvailable(Runtime &, TurboModule &turboModule, const Value *, size_t) {
  return Value(static_cast<const MicButtonModule &>(turboModule).button_ != nullptr);
}

Value MicButtonModule::addListener(Runtime &, TurboModule &turboModule, const Value *, size_t) {
  if (const auto &button = static_cast<const MicButtonModule &>(turboModule).button_) button->claim();
  return Value::undefined();
}

Value MicButtonModule::removeListener(Runtime &, TurboModule &turboModule, const Value *, size_t) {
  if (const auto &button = static_cast<const MicButtonModule &>(turboModule).button_) button->unclaim();
  return Value::undefined();
}

RN_TIZEN_PLATFORM_MODULE("RNTizenMicButton", MicButtonModule);

} // namespace rntizen
