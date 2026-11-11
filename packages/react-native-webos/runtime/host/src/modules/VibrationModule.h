#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `Vibration`, which `Vibration` requires at import. A TV cannot vibrate, so
/// every call does nothing, as on Android TV.
class VibrationModule final : public facebook::react::NativeVibrationCxxSpec<VibrationModule> {
 public:
  VibrationModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  void vibrate(facebook::jsi::Runtime &runtime, double pattern);
  void vibrateByPattern(facebook::jsi::Runtime &runtime, facebook::jsi::Array pattern, double repeat);
  void cancel(facebook::jsi::Runtime &runtime);
};

} // namespace rntizen
