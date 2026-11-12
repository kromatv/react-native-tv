#include "../ExpoTestRuntime.test.h"

#include "modules/LaunchState.h"

#include <doctest/doctest.h>

#include <sys/utsname.h>
#include <unistd.h>

namespace rnwebos::expo {

namespace {

ModuleContext hostWith(const char *model, const char *version, const char *build) {
  ModuleContext host;
  host.device.modelName = model;
  host.device.platformVersion = version;
  host.device.buildString = build;
  host.device.screenWidth = 3840;
  host.device.screenHeight = 2160;
  return host;
}

} // namespace

TEST_CASE("ExpoDevice describes an LG TV from the host's device profile") {
  testing::ExpoTestRuntime expo(hostWith("OLED65C25LB", "10.2.0", "33.22.52"));
  expo.eval("globalThis.D = expo.modules.ExpoDevice");
  CHECK(expo.json("[D.isDevice, D.brand, D.manufacturer, D.deviceType, D.osName]") ==
        R"([true,"LG","LG Electronics",4,"webOS"])");
  CHECK(expo.json("[D.modelName, D.osVersion, D.osBuildId, D.osInternalBuildId, D.deviceName]") ==
        R"(["OLED65C25LB","10.2.0","33.22.52","33.22.52","OLED65C25LB"])");
  CHECK(expo.json("[D.modelId, D.designName, D.productName, D.deviceYearClass, D.osBuildFingerprint, D.platformApiLevel]") ==
        "[null,null,null,null,null,null]");
  const auto memory = static_cast<double>(sysconf(_SC_PHYS_PAGES)) * static_cast<double>(sysconf(_SC_PAGESIZE));
  CHECK(expo.eval("D.totalMemory").asNumber() == memory);
  utsname system{};
  uname(&system);
  CHECK(expo.json("D.supportedCpuArchitectures") == std::string("[\"") + system.machine + "\"]");
}

TEST_CASE("ExpoDevice answers its asynchronous questions as a TV with no side loading") {
  testing::ExpoTestRuntime expo;
  expo.eval("globalThis.D = expo.modules.ExpoDevice");
  CHECK(expo.settle("D.getDeviceTypeAsync()") == "4");
  CHECK(expo.settle("D.isRootedExperimentalAsync()") == "false");
  CHECK(expo.settle("D.isSideLoadingEnabledAsync()") == "false");
  CHECK(expo.settle("D.getPlatformFeaturesAsync()") == "[]");
  CHECK(expo.settle("D.hasPlatformFeatureAsync('android.software.leanback')") == "false");
  const auto first = std::stod(expo.settle("D.getUptimeAsync()"));
  const auto second = std::stod(expo.settle("D.getUptimeAsync()"));
  CHECK(first > 0);
  CHECK(second >= first);
}

TEST_CASE("ExpoWebOS hands JavaScript the model, the platform, the language, the screen and the panel") {
  auto host = hostWith("OLED65C25LB", "10.2.0", "33.22.52");
  host.device.screenWidth = 1920;
  host.device.screenHeight = 1080;
  host.device.panelWidth = 3840;
  host.device.panelHeight = 2160;
  host.device.locale = "en-GB";
  testing::ExpoTestRuntime expo(host);
  expo.eval("globalThis.T = expo.modules.ExpoWebOS");
  CHECK(expo.json("[T.modelName, T.platformVersion, T.buildString, T.locale]") ==
        R"(["OLED65C25LB","10.2.0","33.22.52","en-GB"])");
  CHECK(expo.json("[T.getScreenSize().width, T.getScreenSize().height]") == "[1920,1080]");
  CHECK(expo.json("[T.getPanelSize().width, T.getPanelSize().height]") == "[3840,2160]");
}

TEST_CASE("ExpoWebOS gives the parameters of the launch, then of the last relaunch") {
  auto launch = std::make_shared<LaunchState>();
  launch->setParameters(folly::dynamic::object("contentTarget", "example://title/42"));
  ModuleContext host;
  host.launch = launch;
  testing::ExpoTestRuntime expo(host);
  expo.eval("globalThis.T = expo.modules.ExpoWebOS");
  CHECK(expo.json("T.getLaunchParameters()") == R"({"contentTarget":"example://title/42"})");

  launch->setParameters(folly::dynamic::object("customParam1", "value1"));
  CHECK(expo.json("T.getLaunchParameters()") == R"({"customParam1":"value1"})");
  testing::ExpoTestRuntime withoutLaunch;
  CHECK(withoutLaunch.json("expo.modules.ExpoWebOS.getLaunchParameters()") == "{}");
}

} // namespace rnwebos::expo
