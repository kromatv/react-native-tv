#include "../ExpoModuleRegistry.h"
#include "../ModuleDefinition.h"

namespace rntizen::expo {

namespace {

class ExpoTizenModule final : public Module {
 public:
  ModuleDefinition definition() override {
    const auto &device = context().host().device;
    return ModuleDefinition("ExpoTizen")
        .constant("modelName", [&device] { return device.modelName; })
        .constant("platformVersion", [&device] { return device.platformVersion; })
        .constant("buildString", [&device] { return device.buildString; })
        .function("getScreenSize", [&device]() -> folly::dynamic {
          return folly::dynamic::object("width", device.screenWidth)("height", device.screenHeight);
        });
  }
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoTizen, ExpoTizenModule);

} // namespace rntizen::expo
