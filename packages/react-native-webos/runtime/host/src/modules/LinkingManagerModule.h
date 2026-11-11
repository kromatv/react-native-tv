#pragma once

#include "LaunchState.h"
#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>
#include <react/bridging/Promise.h>

namespace rnwebos {

/// `LinkingManager`, which `Linking` asks for on every platform but Android.
/// `getInitialURL` answers the deep link the TV launched the app with, and one
/// it launches the running app with arrives as the `url` event. `openURL`
/// hands an http or https URL to the TV's browser through the app manager
/// (docs/reference/webos-tv-platform.md, section 4.3); nothing else can be opened.
class LinkingManagerModule final : public facebook::react::NativeLinkingManagerCxxSpec<LinkingManagerModule> {
 public:
  LinkingManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Value getInitialURL(facebook::jsi::Runtime &runtime);
  facebook::jsi::Value canOpenURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url);
  facebook::react::AsyncPromise<> openURL(facebook::jsi::Runtime &runtime, facebook::jsi::String url);
  facebook::jsi::Value openSettings(facebook::jsi::Runtime &runtime);
  void addListener(facebook::jsi::Runtime &runtime, facebook::jsi::String eventName);
  void removeListeners(facebook::jsi::Runtime &runtime, double count);

 private:
  bool opens(const std::string &url) const;

  std::shared_ptr<const LaunchState> launch_;
  LunaBus *const luna_;
};

} // namespace rnwebos
