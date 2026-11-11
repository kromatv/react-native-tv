#pragma once

#include "LaunchUrl.h"
#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `LinkingManager`, which `Linking` asks for on every platform but Android.
/// `getInitialURL` answers the URI of the launching app control, and a later
/// app control arrives as the `url` event. Opening a URL in another app is not
/// supported yet: `canOpenURL` answers false and `openURL` rejects.
class LinkingManagerModule final : public facebook::react::NativeLinkingManagerCxxSpec<LinkingManagerModule> {
 public:
  LinkingManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Value getInitialURL(facebook::jsi::Runtime &runtime);
  facebook::jsi::Value canOpenURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url);
  facebook::jsi::Value openURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url);
  facebook::jsi::Value openSettings(facebook::jsi::Runtime &runtime);
  void addListener(facebook::jsi::Runtime &runtime, facebook::jsi::String eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);

 private:
  std::shared_ptr<const LaunchUrl> launchUrl_;
};

} // namespace rntizen
