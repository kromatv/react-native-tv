#pragma once

#include "modules/ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

#include <memory>
#include <string>

namespace rntizen {

/// `DevSettings`, which every `__DEV__` bundle requires at startup through
/// HMRClient. Reload and Fast Refresh go to the app's DevSupport; Tizen has no
/// developer menu, shake or profiler, so those calls do nothing.
class DevSettingsModule final : public facebook::react::NativeDevSettingsCxxSpec<DevSettingsModule> {
 public:
  DevSettingsModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void reload(facebook::jsi::Runtime &runtime);
  void reloadWithReason(facebook::jsi::Runtime &runtime, std::string reason);
  void onFastRefresh(facebook::jsi::Runtime &runtime);
  void setHotLoadingEnabled(facebook::jsi::Runtime &runtime, bool enabled);
  void setProfilingEnabled(facebook::jsi::Runtime &runtime, bool enabled);
  void toggleElementInspector(facebook::jsi::Runtime &runtime);
  void addMenuItem(facebook::jsi::Runtime &runtime, const std::string &title);
  void openDebugger(facebook::jsi::Runtime &runtime);
  void addListener(facebook::jsi::Runtime &runtime, const std::string &eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);
  void setIsShakeToShowDevMenuEnabled(facebook::jsi::Runtime &runtime, bool enabled);
};

} // namespace rntizen
