#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `DeviceEventManager`: when no `BackHandler` listener consumes the remote's
/// Back key, React Native asks for the default, which on a TV leaves the app.
class DeviceEventManagerModule final
    : public facebook::react::NativeDeviceEventManagerCxxSpec<DeviceEventManagerModule> {
 public:
  DeviceEventManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void invokeDefaultBackPressHandler(facebook::jsi::Runtime &runtime);

 private:
  std::function<void()> exitApp_;
};

} // namespace rntizen
