#include "DeviceInfoModule.h"

#include "DeviceProfile.h"
#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <jsi/JSIDynamic.h>

#include <cstdlib>

namespace rntizen {

namespace {

folly::dynamic dimensions(int width, int height) {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ModuleContext context{};
  context.device = {.modelName = "", .platformVersion = "", .buildString = "", .screenWidth = width, .screenHeight = height};
  DeviceInfoModule module(std::make_shared<testing::QueuedInvoker>(), context);
  return facebook::jsi::dynamicFromValue(rt, testing::call(rt, module, "getConstants"))["Dimensions"];
}

} // namespace

TEST_CASE("DeviceInfo lays every screen out as the logical 1920x1080 one, scaled to the TV's") {
  const auto fullHd = dimensions(1920, 1080);
  const auto hd = dimensions(1280, 720);
  const auto uhd = dimensions(3840, 2160);

  CHECK(fullHd["window"] == fullHd["screen"]);
  CHECK(fullHd["window"]["width"].asDouble() == 1920);
  CHECK(fullHd["window"]["height"].asDouble() == 1080);
  CHECK(fullHd["window"]["scale"].asDouble() == 1);
  CHECK(fullHd["window"]["fontScale"].asDouble() == 1);
  CHECK(fullHd["window"]["densityDpi"].asDouble() == 160);
  CHECK(hd["window"]["width"].asDouble() == 1920);
  CHECK(hd["window"]["scale"].asDouble() == doctest::Approx(2.0 / 3.0));
  CHECK(hd["screen"]["densityDpi"].asDouble() == doctest::Approx(160.0 * 2 / 3));
  CHECK(uhd["screen"]["scale"].asDouble() == 2);
}

TEST_CASE("readDeviceProfile reads the model, the platform and the screen from system info") {
  const auto profile = readDeviceProfile();
  CHECK(profile.modelName == "Simulator");
  CHECK(profile.platformVersion == "9.0");
  CHECK(profile.buildString == "rn-tizen-simulator");
  CHECK(profile.screenWidth == 1920);
  CHECK(profile.screenHeight == 1080);

  setenv("RN_TIZEN_SIMULATOR_SCREEN_SIZE", "1280x720", 1);
  const auto smaller = readDeviceProfile();
  unsetenv("RN_TIZEN_SIMULATOR_SCREEN_SIZE");
  CHECK(smaller.screenWidth == 1280);
  CHECK(smaller.screenHeight == 720);
}

} // namespace rntizen
