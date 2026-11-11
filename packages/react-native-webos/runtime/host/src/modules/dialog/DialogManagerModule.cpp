#include "DialogManagerModule.h"

#include "../PlatformModuleRegistry.h"
#include "AlertDialog.h"

namespace rntizen {

namespace {

constexpr const char *kButtonClicked = "buttonClicked";
constexpr const char *kDismissed = "dismissed";
constexpr int kButtonPositive = -1;
constexpr int kButtonNegative = -2;
constexpr int kButtonNeutral = -3;

std::optional<std::string> stringProperty(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Object &object,
    const char *name) {
  auto value = object.getProperty(runtime, name);
  if (!value.isString()) return std::nullopt;
  return value.getString(runtime).utf8(runtime);
}

} // namespace

DialogManagerModule::DialogManagerModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeDialogManagerAndroidCxxSpec(std::move(jsInvoker)), showAlert_(context.showAlert) {}

facebook::jsi::Object DialogManagerModule::getConstants(facebook::jsi::Runtime &runtime) {
  facebook::jsi::Object constants(runtime);
  constants.setProperty(runtime, "buttonClicked", kButtonClicked);
  constants.setProperty(runtime, "dismissed", kDismissed);
  constants.setProperty(runtime, "buttonPositive", kButtonPositive);
  constants.setProperty(runtime, "buttonNegative", kButtonNegative);
  constants.setProperty(runtime, "buttonNeutral", kButtonNeutral);
  return constants;
}

void DialogManagerModule::showAlert(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::Object config,
    facebook::react::AsyncCallback<std::string> onError,
    facebook::react::AsyncCallback<std::string, std::optional<int>> onAction) {
  if (config.getProperty(runtime, "items").isObject()) {
    onError("Alert items are not supported on Tizen");
    return;
  }
  AlertRequest request{
      .title = stringProperty(runtime, config, "title").value_or(""),
      .message = stringProperty(runtime, config, "message").value_or(""),
      .buttons = {},
      .cancelable = config.getProperty(runtime, "cancelable").isBool() &&
          config.getProperty(runtime, "cancelable").getBool(),
      .onClose =
          [onAction](std::optional<int> buttonKey) {
            if (buttonKey.has_value()) {
              onAction(kButtonClicked, buttonKey);
            } else {
              onAction(kDismissed, std::nullopt);
            }
          },
  };
  for (auto [name, key] : {std::pair{"buttonNeutral", kButtonNeutral},
                           std::pair{"buttonNegative", kButtonNegative},
                           std::pair{"buttonPositive", kButtonPositive}}) {
    if (auto text = stringProperty(runtime, config, name)) request.buttons.push_back({.text = *text, .key = key});
  }
  if (showAlert_) showAlert_(std::move(request));
}

RN_TIZEN_PLATFORM_MODULE("DialogManagerAndroid", DialogManagerModule);

} // namespace rntizen
