#include "DeviceInfoModule.h"

#include "../mounting/LogicalScreen.h"
#include "PlatformModuleRegistry.h"

#include <jsi/JSIDynamic.h>

namespace rntizen {

DeviceInfoModule::DeviceInfoModule(
    std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
    const ModuleContext &context)
    : NativeDeviceInfoCxxSpec(std::move(jsInvoker)), device_(context.device) {}

facebook::jsi::Object DeviceInfoModule::getConstants(facebook::jsi::Runtime &runtime) {
  const auto scale = pointScaleFactor(
      {.width = static_cast<facebook::react::Float>(device_.screenWidth),
       .height = static_cast<facebook::react::Float>(device_.screenHeight)});
  folly::dynamic metrics = folly::dynamic::object("width", kLogicalScreenSize.width)(
      "height", kLogicalScreenSize.height)("scale", scale)("fontScale", 1)("densityDpi", 160 * scale);
  folly::dynamic constants =
      folly::dynamic::object("Dimensions", folly::dynamic::object("window", metrics)("screen", metrics));
  return facebook::jsi::valueFromDynamic(runtime, constants).asObject(runtime);
}

RN_TIZEN_PLATFORM_MODULE("DeviceInfo", DeviceInfoModule);

} // namespace rntizen
