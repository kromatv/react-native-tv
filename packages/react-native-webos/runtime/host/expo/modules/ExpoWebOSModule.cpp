#include "../ExpoModuleRegistry.h"
#include "../ModuleDefinition.h"

namespace rnwebos::expo {

namespace {

class ExpoWebOSModule final : public Module {
 public:
  ModuleDefinition definition() override {
    const auto &device = context().host().device;
    return ModuleDefinition("ExpoWebOS")
        .constant("modelName", [&device] { return device.modelName; })
        .constant("platformVersion", [&device] { return device.platformVersion; })
        .constant("buildString", [&device] { return device.buildString; })
        .function("getScreenSize", [&device]() -> folly::dynamic {
          return folly::dynamic::object("width", device.screenWidth)("height", device.screenHeight);
        });
  }
};

} // namespace

RN_WEBOS_EXPO_MODULE(ExpoWebOS, ExpoWebOSModule);

} // namespace rnwebos::expo
