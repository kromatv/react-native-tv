#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `Appearance`: a TV has no system light mode, so the color scheme is `dark`
/// until the app picks one with `Appearance.setColorScheme`.
class AppearanceModule final : public facebook::react::NativeAppearanceCxxSpec<AppearanceModule> {
 public:
  AppearanceModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::String getColorScheme(facebook::jsi::Runtime &runtime);
  void setColorScheme(facebook::jsi::Runtime &runtime, facebook::jsi::String colorScheme);
  void addListener(facebook::jsi::Runtime &runtime, facebook::jsi::String eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);

 private:
  std::string colorScheme_{"dark"};
};

} // namespace rntizen
