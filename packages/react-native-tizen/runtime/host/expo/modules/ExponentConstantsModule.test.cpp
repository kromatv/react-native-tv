#include "../ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <regex>
#include <string>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

/// Writes `text` as the package's app.config for one test, and removes it after.
class AppConfig final {
 public:
  explicit AppConfig(const std::string &text) {
    std::ofstream(file_, std::ios::binary) << text;
  }
  ~AppConfig() {
    std::error_code ignored;
    fs::remove(file_, ignored);
  }
  AppConfig(const AppConfig &) = delete;
  AppConfig &operator=(const AppConfig &) = delete;

 private:
  fs::path file_ = testing::testAppRoot() / "res" / "app.config";
};

ModuleContext tv() {
  ModuleContext host;
  host.device.modelName = "QE55Q80";
  host.device.platformVersion = "9.0";
  return host;
}

} // namespace

TEST_CASE("ExponentConstants describes a bare app on a TV, with no manifest when the package has none") {
  testing::ExpoTestRuntime expo(tv());
  expo.eval("globalThis.C = expo.modules.ExponentConstants");
  CHECK(expo.json("[C.manifest, C.executionEnvironment, C.statusBarHeight, C.deviceName, C.systemFonts]") ==
        R"([null,"bare",0,"QE55Q80",[]])");
  CHECK(expo.json("[C.systemVersion, C.debugMode, C.isHeadless]") == R"(["9.0",false,false])");
  CHECK(expo.settle("C.getWebViewUserAgentAsync()") == "null");
  const std::regex uuid(R"("[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}")");
  const auto session = expo.json("C.sessionId");
  CHECK(std::regex_match(session, uuid));
  testing::ExpoTestRuntime again(tv());
  CHECK(again.json("expo.modules.ExponentConstants.sessionId") == session);
}

TEST_CASE("ExponentConstants reads the package's app.config as the manifest") {
  AppConfig config(R"({"name":"Example","slug":"example","extra":{"buildInfo":{"commit":"abc"}}})");
  testing::ExpoTestRuntime expo(tv());
  CHECK(expo.json("[expo.modules.ExponentConstants.manifest.name, expo.modules.ExponentConstants.manifest.extra.buildInfo.commit]") ==
        R"(["Example","abc"])");
}

TEST_CASE("ExponentConstants ignores an app.config that is not a JSON object") {
  {
    AppConfig config("[1, 2, 3]");
    testing::ExpoTestRuntime expo(tv());
    CHECK(expo.json("expo.modules.ExponentConstants.manifest") == "null");
  }
  {
    AppConfig config("{ not json");
    testing::ExpoTestRuntime expo(tv());
    CHECK(expo.json("expo.modules.ExponentConstants.manifest") == "null");
  }
}

} // namespace rntizen::expo
