#include "HostConfig.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rntizen {

namespace {

namespace fs = std::filesystem;

struct ResourceDirectory {
  ResourceDirectory() {
    fs::remove_all(path);
    fs::create_directories(path);
  }
  ~ResourceDirectory() {
    fs::remove_all(path);
  }
  ResourceDirectory(const ResourceDirectory &) = delete;
  ResourceDirectory &operator=(const ResourceDirectory &) = delete;

  HostConfig load(const std::string &json) const {
    std::ofstream(path / "react-native-tizen.json") << json;
    return loadHostConfig(path.string());
  }

  const fs::path path{fs::temp_directory_path() / "rn-tizen-host-config-test"};
};

} // namespace

TEST_CASE_FIXTURE(ResourceDirectory, "loadHostConfig reads every field") {
  const auto config = load(R"({
    "appKey": "main", "bundleFile": "index.bundle", "bundleUrl": "http://10.0.0.2:8081/index.bundle",
    "log": {"host": "10.0.0.2", "port": 9100}, "development": true, "resolution": "4k",
    "backgroundMedia": true, "voice": {"searchUrl": "app://search?q={query}"}
  })");

  CHECK(config.appKey == "main");
  CHECK(config.bundleFile == "index.bundle");
  CHECK(config.bundleUrl == "http://10.0.0.2:8081/index.bundle");
  REQUIRE(config.logEndpoint.has_value());
  CHECK(config.logEndpoint->host == "10.0.0.2");
  CHECK(config.logEndpoint->port == 9100);
  CHECK(config.development);
  CHECK(config.uhd);
  CHECK(config.backgroundMedia);
  CHECK(config.voice);
  CHECK(config.voiceSearchUrl == "app://search?q={query}");
  CHECK(config.resourceDir == path.string());
}

TEST_CASE_FIXTURE(ResourceDirectory, "loadHostConfig defaults every optional field") {
  const auto config = load(
      R"({"appKey": "main", "bundleFile": "index.bundle", "bundleUrl": null, "log": null, "development": null, "voice": null})");
  const auto bare = load(R"({"appKey": "main", "bundleFile": "index.bundle"})");

  CHECK_FALSE(config.bundleUrl.has_value());
  CHECK_FALSE(config.logEndpoint.has_value());
  CHECK_FALSE(config.development);
  CHECK_FALSE(config.uhd);
  CHECK_FALSE(config.voice);
  CHECK_FALSE(config.voiceSearchUrl.has_value());
  CHECK_FALSE(bare.voice);
  CHECK_FALSE(bare.voiceSearchUrl.has_value());
}

TEST_CASE_FIXTURE(ResourceDirectory, "loadHostConfig names what is wrong") {
  const std::string required = R"("appKey": "main", "bundleFile": "index.bundle")";

  CHECK_THROWS_AS(loadHostConfig((path / "missing").string()), HostConfigError);
  CHECK_THROWS_AS(load("[]"), HostConfigError);
  CHECK_THROWS_AS(load(R"({"appKey": "", "bundleFile": "index.bundle"})"), HostConfigError);
  CHECK_THROWS_AS(load(R"({"bundleFile": "index.bundle"})"), HostConfigError);
  CHECK_THROWS_AS(load(R"({"appKey": "main", "bundleFile": 1})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "log": {"host": "10.0.0.2", "port": "9100"}})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "bundleUrl": 8081})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "development": "yes"})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "log": {"host": "10.0.0.2", "port": 0}})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "log": {"host": "10.0.0.2", "port": 70000}})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "log": 9100})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "voice": true})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "resolution": "8k"})"), HostConfigError);
  CHECK_THROWS_AS(load("{" + required + R"(, "padding": ")" + std::string(70 * 1024, 'x') + "\"}"), HostConfigError);
}

} // namespace rntizen
