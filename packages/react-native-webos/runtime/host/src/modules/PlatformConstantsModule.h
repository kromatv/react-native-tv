#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `PlatformConstants`, in the shape of Android's spec: `Platform.constants`
/// reports the TV's model and Tizen version, and `uiMode` is always `tv`.
class PlatformConstantsModule final
    : public facebook::react::NativePlatformConstantsAndroidCxxSpec<PlatformConstantsModule> {
 public:
  PlatformConstantsModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);
  facebook::jsi::String getAndroidID(facebook::jsi::Runtime &runtime);

 private:
  DeviceProfile device_;
};

} // namespace rntizen
