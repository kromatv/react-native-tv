#pragma once

#include "../modules/ModuleContext.h"

#include <ReactCommon/TurboModule.h>

#include <memory>

namespace rntizen {

/// `RNTizenMicButton`: every JavaScript listener holds the Smart Remote's
/// microphone button, and the presses come back as `rnTizenMicButton` events.
class MicButtonModule final : public facebook::react::TurboModule {
 public:
  MicButtonModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

 private:
  using Runtime = facebook::jsi::Runtime;
  using Value = facebook::jsi::Value;

  static Value isAvailable(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value addListener(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value removeListener(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);

  const std::shared_ptr<MicButton> button_;
};

} // namespace rntizen
