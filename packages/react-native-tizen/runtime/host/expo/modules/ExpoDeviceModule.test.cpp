#include "DeviceName.h"

#include "../ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <sys/utsname.h>
#include <unistd.h>

namespace rntizen::expo {

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

TEST_CASE("ExpoDevice describes a Samsung TV from the host's device profile") {
  testing::ExpoTestRuntime expo(hostWith("QE55Q80", "9.0", "T-PTMDEUC-1001.2"));
  expo.eval("globalThis.D = expo.modules.ExpoDevice");
  CHECK(expo.json("[D.isDevice, D.brand, D.manufacturer, D.deviceType, D.osName]") ==
        R"([true,"Samsung","Samsung",4,"Tizen"])");
  CHECK(expo.json("[D.modelName, D.osVersion, D.osBuildId, D.osInternalBuildId, D.deviceName]") ==
        R"(["QE55Q80","9.0","T-PTMDEUC-1001.2","T-PTMDEUC-1001.2","QE55Q80"])");
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

TEST_CASE("ExpoTizen hands JavaScript the model, the platform and the screen") {
  testing::ExpoTestRuntime expo(hostWith("QE55Q80", "9.0", "T-PTMDEUC-1001.2"));
  expo.eval("globalThis.T = expo.modules.ExpoTizen");
  CHECK(expo.json("[T.modelName, T.platformVersion, T.buildString]") == R"(["QE55Q80","9.0","T-PTMDEUC-1001.2"])");
  CHECK(expo.json("[T.getScreenSize().width, T.getScreenSize().height]") == "[3840,2160]");
}

TEST_CASE("tvDeviceName falls back to the model where the platform keeps no name") {
  CHECK(tvDeviceName("QE55Q80") == "QE55Q80");
  CHECK(tvDeviceName("").empty());
}

} // namespace rntizen::expo
