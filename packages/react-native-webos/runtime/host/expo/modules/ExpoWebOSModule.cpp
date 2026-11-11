#include "../ExpoModuleRegistry.h"
#include "../ModuleDefinition.h"

#include "modules/LaunchState.h"

namespace rnwebos::expo {

namespace {

folly::dynamic size(int width, int height) {
  return folly::dynamic::object("width", width)("height", height);
}

class ExpoWebOSModule final : public Module {
 public:
  ModuleDefinition definition() override {
    const auto &host = context().host();
    const auto &device = host.device;
    const auto launch = host.launch;
    return ModuleDefinition("ExpoWebOS")
        .constant("modelName", [&device] { return device.modelName; })
        .constant("platformVersion", [&device] { return device.platformVersion; })
        .constant("buildString", [&device] { return device.buildString; })
        .constant("locale", [&device] { return device.locale; })
        .function("getScreenSize", [&device] { return size(device.screenWidth, device.screenHeight); })
        .function("getPanelSize", [&device] { return size(device.panelWidth, device.panelHeight); })
        .function("getLaunchParameters", [launch]() -> folly::dynamic {
          return launch != nullptr ? launch->parameters() : folly::dynamic(folly::dynamic::object());
        });
  }
};

} // namespace

RN_WEBOS_EXPO_MODULE(ExpoWebOS, ExpoWebOSModule);

} // namespace rnwebos::expo
