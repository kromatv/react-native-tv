#include "DevLoadingViewModule.h"

#include "DevSupport.h"

#include "modules/PlatformModuleRegistry.h"

namespace rntizen {

std::optional<std::uint32_t> argbColor(std::optional<double> color) {
  if (!color.has_value()) return std::nullopt;
  return static_cast<std::uint32_t>(static_cast<std::int64_t>(*color));
}

DevLoadingViewModule::DevLoadingViewModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &)
    : NativeDevLoadingViewCxxSpec(std::move(jsInvoker)) {}

void DevLoadingViewModule::showMessage(
    facebook::jsi::Runtime &,
    std::string message,
    std::optional<double> textColor,
    std::optional<double> backgroundColor,
    std::optional<bool>) {
  DevSupport::post(
      [message = std::move(message), text = argbColor(textColor), background = argbColor(backgroundColor)](
          DevSupport &devSupport) { devSupport.showMessage(message, text, background); });
}

void DevLoadingViewModule::hide(facebook::jsi::Runtime &) {
  DevSupport::post([](DevSupport &devSupport) { devSupport.hideMessage(); });
}

RN_TIZEN_PLATFORM_MODULE("DevLoadingView", DevLoadingViewModule);

} // namespace rntizen
