#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

#include <optional>

namespace rntizen {

/// `StatusBarManager`, which `StatusBar` (and expo-status-bar) require at
/// import. A TV has no status bar: its height is 0 and every setter does nothing.
class StatusBarManagerModule final
    : public facebook::react::NativeStatusBarManagerAndroidCxxSpec<StatusBarManagerModule> {
 public:
  StatusBarManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  void setStyle(facebook::jsi::Runtime &runtime, std::optional<facebook::jsi::String> style);
  void setHidden(facebook::jsi::Runtime &runtime, bool hidden);
};

} // namespace rntizen
