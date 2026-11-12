#pragma once

#include "modules/ModuleContext.h"

#include <FBReactNativeSpec/FBReactNativeSpecJSI.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace rntizen {

/// `DevLoadingView`: the "Refreshing..." banner Fast Refresh shows while it
/// applies an edit. The banner has no dismiss button: a remote cannot press it.
class DevLoadingViewModule final : public facebook::react::NativeDevLoadingViewCxxSpec<DevLoadingViewModule> {
 public:
  DevLoadingViewModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, const ModuleContext &context);

  void showMessage(
      facebook::jsi::Runtime &runtime,
      std::string message,
      std::optional<double> textColor,
      std::optional<double> backgroundColor,
      std::optional<bool> dismissButton);
  void hide(facebook::jsi::Runtime &runtime);
};

/// A color `processColor` gave, signed on Android and unsigned elsewhere, as 0xAARRGGBB.
std::optional<std::uint32_t> argbColor(std::optional<double> color);

} // namespace rntizen
