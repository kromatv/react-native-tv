#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `AccessibilityManager`, the module `AccessibilityInfo` asks for on every
/// platform but Android. Tizen exposes no accessibility settings to a native
/// app, so every setting reads as off and announcements are dropped.
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
  void getCurrentVoiceOverState(facebook::jsi::Runtime &runtime, Callback onSuccess, Callback onError);
  void setAccessibilityContentSizeMultipliers(facebook::jsi::Runtime &runtime, facebook::jsi::Object multipliers);
  void setAccessibilityFocus(facebook::jsi::Runtime &runtime, double reactTag);
  void announceForAccessibility(facebook::jsi::Runtime &runtime, facebook::jsi::String announcement);
  void announceForAccessibilityWithOptions(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::String announcement,
      facebook::jsi::Object options);
};

} // namespace rntizen
