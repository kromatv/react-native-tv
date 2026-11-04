#include "PlatformConstantsModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <jsi/JSIDynamic.h>

namespace rntizen {

using testing::call;

TEST_CASE("PlatformConstants describes the TV in Android's shape, with React Native's version") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ModuleContext context{};
  context.device = {
      .modelName = "QE55Q80", .platformVersion = "9.0", .buildString = "T-PTMDEUC-1234", .screenWidth = 1920, .screenHeight = 1080};
  PlatformConstantsModule module(std::make_shared<testing::QueuedInvoker>(), context);

  const auto constants = facebook::jsi::dynamicFromValue(rt, call(rt, module, "getConstants"));

  CHECK(constants["Version"] == 9);
  CHECK(constants["Release"] == "9.0");
  CHECK(constants["Model"] == "QE55Q80");
  CHECK(constants["Fingerprint"] == "T-PTMDEUC-1234");
  CHECK(constants["Serial"] == "unknown");
  CHECK(constants["uiMode"] == "tv");
  CHECK(constants["Brand"] == "Samsung");
  CHECK(constants["Manufacturer"] == "Samsung");
  CHECK(constants["isTesting"] == false);
  CHECK(constants["reactNativeVersion"]["major"] == 0);
  CHECK(constants["reactNativeVersion"]["minor"] == 88);
  CHECK(constants["reactNativeVersion"]["prerelease"] == "0");
  CHECK(call(rt, module, "getAndroidID").asString(rt).utf8(rt).empty());
}

} // namespace rntizen
