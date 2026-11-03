#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `DeviceInfo`: what `Dimensions` and `useWindowDimensions` report. Every TV
/// window is the logical 1920x1080 screen, at the scale that fits it.
class DeviceInfoModule final : public facebook::react::NativeDeviceInfoCxxSpec<DeviceInfoModule> {
 public:
  DeviceInfoModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &runtime);

 private:
  DeviceProfile device_;
};

} // namespace rntizen
