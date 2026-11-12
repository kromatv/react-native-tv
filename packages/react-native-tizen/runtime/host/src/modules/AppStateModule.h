#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `AppState`: `active` in the foreground, `background` once the TV pauses the
/// app. The host emits `appStateDidChange` when that flips.
class AppStateModule final : public facebook::react::NativeAppStateCxxSpec<AppStateModule> {
 public:
  AppStateModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  void getCurrentAppState(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::Function success,
      facebook::jsi::Function error);
  void addListener(facebook::jsi::Runtime &runtime, facebook::jsi::String eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);

  static const char *stateName(bool isActive);

 private:
  std::shared_ptr<std::atomic<bool>> isActive_;
};

} // namespace rntizen
