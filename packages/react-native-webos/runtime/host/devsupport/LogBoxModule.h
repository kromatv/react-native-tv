#pragma once

#include "modules/ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

#include <memory>

namespace rntizen {

/// `LogBox`: JavaScript asks for its inspector to be shown or hidden, and the
/// host runs or stops the `LogBox` surface over the app's, as Android's
/// LogBoxModule does in a dialog.
class LogBoxModule final : public facebook::react::NativeLogBoxCxxSpec<LogBoxModule> {
 public:
  LogBoxModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void show(facebook::jsi::Runtime &runtime);
  void hide(facebook::jsi::Runtime &runtime);
};

} // namespace rntizen
