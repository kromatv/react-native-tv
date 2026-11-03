#pragma once

#include "ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

namespace rntizen {

/// `ExceptionsManager`: JavaScript errors reach the log with their stack, and a
/// fatal one takes the host's fatal path, as an error before the bundle ran
/// does: the red screen in development, leaving the app in release.
class ExceptionsManagerModule final
    : public facebook::react::NativeExceptionsManagerCxxSpec<ExceptionsManagerModule> {
 public:
  ExceptionsManagerModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void reportFatalException(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::String message,
      facebook::jsi::Array stack,
      double exceptionId);
  void reportSoftException(
      facebook::jsi::Runtime &runtime,
      facebook::jsi::String message,
      facebook::jsi::Array stack,
      double exceptionId);
  void reportException(facebook::jsi::Runtime &runtime, facebook::jsi::Object data);
  void dismissRedbox(facebook::jsi::Runtime &runtime);

 private:
  void report(const ErrorReport &report, bool isFatal);

  std::function<void(const ErrorReport &report)> reportFatalError_;
};

} // namespace rntizen
