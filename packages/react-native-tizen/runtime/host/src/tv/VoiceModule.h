#pragma once

#include "../modules/ModuleContext.h"

#include <ReactCommon/TurboModule.h>

#include <memory>

namespace rntizen {

/// `RNTizenVoice`: whether Samsung's voice service took the app, the screen
/// JavaScript says it shows, and how many listeners each voice event has.
class VoiceModule final : public facebook::react::TurboModule {
 public:
  VoiceModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

 private:
  using Runtime = facebook::jsi::Runtime;
  using Value = facebook::jsi::Value;

  static TvState *state(const TurboModule &turboModule);
  static Value isAvailable(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value setScreen(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value changeListeners(
      Runtime &runtime, const TurboModule &turboModule, const Value *arguments, size_t count, int delta);
  static Value addListener(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);
  static Value removeListener(Runtime &runtime, TurboModule &turboModule, const Value *arguments, size_t count);

  const std::shared_ptr<TvState> state_;
};

} // namespace rntizen
