#include "DeviceProfile.h"

#include "../platform/FakeLuna.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdlib>

namespace rnwebos {

namespace {

constexpr const char *kSystemInfo = "luna://com.webos.service.tv.systemproperty/getSystemInfo";
constexpr const char *kSettings = "luna://com.webos.settingsservice/getSystemSettings";

constexpr const char *kLocaleInfo =
    R"({"method":"getSystemSettings","settings":{"localeInfo":{"locales":{"UI":"en-GB","TV":"en-GB","FMT":"en-GB",)"
    R"("NLP":"en-GB","STT":"fr-FR","AUD":"fr-FR","AUD2":"en-GB"},"clock":"locale","keyboards":["en"],"timezone":""}},)"
    R"("returnValue":true})";

DeviceProfile placeholders() {
  return {.modelName = "LG TV", .platformVersion = "unknown", .buildString = "unknown", .screenWidth = 1920, .screenHeight = 1080};
}

auto soon() {
  return std::chrono::steady_clock::now() + std::chrono::milliseconds(20);
}

} // namespace

TEST_CASE("before the TV answers, the profile is the simulator's environment or placeholders") {
  for (const auto *name : {"RN_WEBOS_MODEL_NAME", "RN_WEBOS_PLATFORM_VERSION", "RN_WEBOS_BUILD", "RN_WEBOS_LOCALE"}) unsetenv(name);
  const auto bare = readDeviceProfile();
  CHECK(bare.modelName == "LG TV");
  CHECK(bare.platformVersion == "unknown");
  CHECK(bare.buildString == "unknown");
  CHECK(bare.screenWidth == 1920);
  CHECK(bare.screenHeight == 1080);
  CHECK(bare.locale.empty());

  setenv("RN_WEBOS_MODEL_NAME", "Simulator", 1);
  setenv("RN_WEBOS_PLATFORM_VERSION", "simulator", 1);
  setenv("RN_WEBOS_BUILD", "rn-webos-simulator", 1);
  setenv("RN_WEBOS_LOCALE", "fr-FR", 1);
  const auto simulated = readDeviceProfile();
  for (const auto *name : {"RN_WEBOS_MODEL_NAME", "RN_WEBOS_PLATFORM_VERSION", "RN_WEBOS_BUILD", "RN_WEBOS_LOCALE"}) unsetenv(name);
  CHECK(simulated.modelName == "Simulator");
  CHECK(simulated.platformVersion == "simulator");
  CHECK(simulated.buildString == "rn-webos-simulator");
  CHECK(simulated.locale == "fr-FR");
}

TEST_CASE("getSystemInfo gives the model, the webOS version, the firmware and whether the panel is Ultra HD") {
  auto profile = placeholders();

  CHECK(applySystemInfo(
      profile,
      folly::parseJson(R"({"returnValue":true,"modelName":"WEBOS1","firmwareVersion":"3.00.00","UHD":"true","sdkVersion":"1.05.00"})")));

  CHECK(profile.modelName == "WEBOS1");
  CHECK(profile.platformVersion == "1.05.00");
  CHECK(profile.buildString == "3.00.00");
  CHECK(profile.panelWidth == 3840);
  CHECK(profile.panelHeight == 2160);
  CHECK(profile.screenWidth == 1920);

  CHECK(applySystemInfo(profile, folly::parseJson(R"({"returnValue":true,"UHD":"false"})")));
  CHECK(profile.panelWidth == 1920);
  CHECK(profile.panelHeight == 1080);
  CHECK(profile.modelName == "WEBOS1");
}

TEST_CASE("a refused or malformed getSystemInfo leaves the profile as it was") {
  auto profile = placeholders();

  CHECK_FALSE(applySystemInfo(
      profile, folly::parseJson(R"({"returnValue":false,"errorCode":"ERROR_06","errorText":"Invalid argument."})")));
  CHECK_FALSE(applySystemInfo(profile, folly::parseJson("[]")));
  CHECK(applySystemInfo(
      profile,
      folly::parseJson(R"({"returnValue":true,"modelName":42,"sdkVersion":"","firmwareVersion":")" + std::string(300, '1') + "\"}")));

  CHECK(profile.modelName == "LG TV");
  CHECK(profile.platformVersion == "unknown");
  CHECK(profile.buildString == "unknown");
  CHECK(profile.panelWidth == 0);
}

TEST_CASE("localeInfo gives the language of the TV's menus") {
  auto profile = placeholders();

  CHECK(applyLocaleInfo(profile, folly::parseJson(kLocaleInfo)));
  CHECK(profile.locale == "en-GB");

  CHECK_FALSE(applyLocaleInfo(
      profile, folly::parseJson(R"({"returnValue":false,"errorText":"There is no matched result from DB"})")));
  CHECK_FALSE(applyLocaleInfo(profile, folly::parseJson(R"({"returnValue":true,"settings":{"localeInfo":{"locales":{}}}})")));
  CHECK_FALSE(applyLocaleInfo(profile, folly::parseJson(R"({"returnValue":true,"settings":{}})")));
  CHECK(profile.locale == "en-GB");
}

TEST_CASE("the profile request asks both questions at once and takes what came by the deadline") {
  testing::FakeLuna luna;
  luna.answer(kSystemInfo, R"({"returnValue":true,"modelName":"OLED65C25LB","firmwareVersion":"43.21.74","UHD":"true","sdkVersion":"11.2.0"})");
  luna.answer(kSettings, kLocaleInfo);
  LunaBus bus(luna.transport());
  auto profile = placeholders();

  const DeviceProfileRequest request(bus);
  REQUIRE(luna.calls().size() == 2);
  CHECK(luna.callsTo(kSystemInfo)[0] ==
        folly::dynamic::object("keys", folly::dynamic::array("modelName", "firmwareVersion", "sdkVersion", "UHD")));
  CHECK(luna.callsTo(kSettings)[0] == folly::dynamic::object("keys", folly::dynamic::array("localeInfo")));
  request.finish(profile, soon());

  CHECK(profile.modelName == "OLED65C25LB");
  CHECK(profile.platformVersion == "11.2.0");
  CHECK(profile.buildString == "43.21.74");
  CHECK(profile.locale == "en-GB");
}

TEST_CASE("a TV that does not answer in time, or no TV at all, leaves the placeholders") {
  testing::FakeLuna silent;
  LunaBus silentBus(silent.transport());
  auto profile = placeholders();
  const auto started = std::chrono::steady_clock::now();
  DeviceProfileRequest(silentBus).finish(profile, soon());
  CHECK(std::chrono::steady_clock::now() - started < std::chrono::milliseconds(500));
  CHECK(profile.modelName == "LG TV");
  CHECK(profile.locale.empty());

  testing::FakeLuna absent;
  absent.setAvailable(false);
  LunaBus absentBus(absent.transport());
  DeviceProfileRequest(absentBus).finish(profile, std::chrono::steady_clock::now() + std::chrono::seconds(30));
  CHECK(profile.modelName == "LG TV");
  CHECK(absent.calls().empty());
}

} // namespace rnwebos
