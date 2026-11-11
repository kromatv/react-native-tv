#pragma once

#include "ModuleContext.h"

#include "../platform/LunaBus.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>
#include <react/bridging/Function.h>

namespace rnwebos {

/// `AccessibilityManager`, the module `AccessibilityInfo` asks for on every
/// platform but Android. The screen reader is the TV's audio guidance, read
/// from its settings and followed as `screenReaderChanged`
/// (docs/reference/webos-tv-platform.md, section 4.3). webOS tells an app no
/// other accessibility setting, so the rest read as off and announcements are dropped.
class AccessibilityManagerModule final
    : public facebook::react::NativeAccessibilityManagerCxxSpec<AccessibilityManagerModule> {
 public:
  using Callback = facebook::jsi::Function;

  AccessibilityManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void getCurrentBoldTextState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void getCurrentGrayscaleState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void getCurrentInvertColorsState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void getCurrentReduceMotionState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void getCurrentDarkerSystemColorsState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void getCurrentPrefersCrossFadeTransitionsState(
      facebook::jsi::Runtime &runtime,
      Callback onSuccess,
      Callback onError);
  void getCurrentReduceTransparencyState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void getCurrentVoiceOverState(
      facebook::jsi::Runtime &runtime,
      facebook::react::AsyncCallback<bool> onSuccess,
      Callback onError);
  void setAccessibilityContentSizeMultipliers(facebook::jsi::Runtime &runtime, facebook::jsi::Object multipliers);
  void setAccessibilityFocus(facebook::jsi::Runtime &runtime, double reactTag);
  void announceForAccessibility(facebook::jsi::Runtime &runtime, facebook::jsi::String announcement);
  void announceForAccessibilityWithOptions(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::String announcement,
      facebook::jsi::Object options);

 private:
  LunaBus *const luna_;
  std::unique_ptr<LunaBus::Subscription> audioGuidance_;
};

/// Whether a `getSystemSettings` reply for `audioGuidance` says the guidance is on; nothing when it does not say.
std::optional<bool> audioGuidanceOf(const folly::dynamic &reply);

} // namespace rnwebos
