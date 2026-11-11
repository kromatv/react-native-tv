#include "DeviceInfoModule.h"

#include "DeviceProfile.h"
#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <jsi/JSIDynamic.h>

#include <map>
#include <string>

namespace rnwebos {

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

TEST_CASE("readDeviceProfile reads the model, the platform and the build from the environment") {
  const std::map<std::string, std::string, std::less<>> variables{
      {"RN_WEBOS_MODEL_NAME", "OLED55C3"},
      {"RN_WEBOS_PLATFORM_VERSION", "8.3.0"},
      {"RN_WEBOS_BUILD", "03.33.85"},
  };

  const auto profile = readDeviceProfile([&variables](const char *name) {
    const auto found = variables.find(name);
    return found == variables.end() ? nullptr : found->second.c_str();
  });

  CHECK(profile.modelName == "OLED55C3");
  CHECK(profile.platformVersion == "8.3.0");
  CHECK(profile.buildString == "03.33.85");
  CHECK(profile.screenWidth == 1920);
  CHECK(profile.screenHeight == 1080);
}

TEST_CASE("readDeviceProfile names an LG TV of an unknown version when the environment says nothing") {
  const auto unset = readDeviceProfile([](const char *) -> const char * { return nullptr; });
  const auto empty = readDeviceProfile([](const char *) -> const char * { return ""; });

  CHECK(unset.modelName == "LG TV");
  CHECK(unset.platformVersion == "unknown");
  CHECK(unset.buildString == "unknown");
  CHECK(empty.modelName == "LG TV");
  CHECK(empty.platformVersion == "unknown");
}

} // namespace rnwebos
