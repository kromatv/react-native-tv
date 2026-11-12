#include "PlatformConstantsModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <jsi/JSIDynamic.h>

namespace rnwebos {

using testing::call;

TEST_CASE("PlatformConstants describes the TV in Android's shape, with React Native's version") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  ModuleContext context{};
  context.device = {
      .modelName = "OLED65C25LB", .platformVersion = "10.2.0", .buildString = "33.22.52", .screenWidth = 1920, .screenHeight = 1080};
  PlatformConstantsModule module(std::make_shared<testing::QueuedInvoker>(), context);

  const auto constants = facebook::jsi::dynamicFromValue(rt, call(rt, module, "getConstants"));

  CHECK(constants["Version"].asDouble() == doctest::Approx(10.02));
  CHECK(constants["Release"] == "10.2.0");
  CHECK(constants["Model"] == "OLED65C25LB");
  CHECK(constants["Fingerprint"] == "33.22.52");
  CHECK(constants["Serial"] == "unknown");
  CHECK(constants["uiMode"] == "tv");
  CHECK(constants["Brand"] == "LG");
  CHECK(constants["Manufacturer"] == "LG Electronics");
  CHECK(constants["isTesting"] == false);
  CHECK(constants["reactNativeVersion"]["major"] == 0);
  CHECK(constants["reactNativeVersion"]["minor"] == 88);
  CHECK(constants["reactNativeVersion"]["prerelease"] == "0");
  CHECK(call(rt, module, "getAndroidID").asString(rt).utf8(rt).empty());
}

} // namespace rnwebos
