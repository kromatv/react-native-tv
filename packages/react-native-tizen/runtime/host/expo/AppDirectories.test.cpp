#include "AppDirectories.h"

#include "ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

class ScopedVariable final {
 public:
  ScopedVariable(const char *name, const std::optional<std::string> &value) : name_(name) {
    if (const char *previous = std::getenv(name)) previous_ = previous;
    if (value) {
      setenv(name, value->c_str(), 1);
    } else {
      unsetenv(name);
    }
  }

  ~ScopedVariable() {
    if (previous_) {
      setenv(name_, previous_->c_str(), 1);
    } else {
      unsetenv(name_);
    }
  }

  ScopedVariable(const ScopedVariable &) = delete;
  ScopedVariable &operator=(const ScopedVariable &) = delete;

 private:
  const char *name_;
  std::optional<std::string> previous_;
};

std::string withSlash(const fs::path &path) {
  return path.string() + "/";
}

} // namespace

TEST_CASE("appDirectories answers the app's data, cache and resource directories with a trailing slash") {
  const auto &directories = appDirectories();
  CHECK(directories.data == withSlash(testing::testAppRoot() / "data"));
  CHECK(directories.cache == withSlash(testing::testAppRoot() / "cache"));
  CHECK(directories.resource == withSlash(testing::testAppRoot() / "res"));
  CHECK(fs::is_directory(directories.data));
  CHECK(fs::is_directory(directories.cache));
  CHECK(&appDirectories() == &directories);
}

TEST_CASE("readAppDirectories falls back to a private directory of its own when app_common has no path") {
  const auto blocker = testing::testAppRoot() / "not-a-directory";
  std::ofstream(blocker).close();
  const ScopedVariable dataVariable("RN_TIZEN_SIMULATOR_DATA_DIR", (blocker / "data").string());
  const ScopedVariable cacheVariable("RN_TIZEN_SIMULATOR_CACHE_DIR", (blocker / "cache").string());
  const ScopedVariable resourceVariable("RN_TIZEN_SIMULATOR_RESOURCE_DIR", std::nullopt);

  const auto directories = readAppDirectories();
  const fs::path data(directories.data);
  const auto privateRoot = data.parent_path().parent_path();
  CHECK(data.filename().empty());
  CHECK(data.parent_path().filename() == "data");
  CHECK(directories.cache == withSlash(privateRoot / "cache"));
  CHECK(fs::equivalent(privateRoot.parent_path(), fs::temp_directory_path()));
  CHECK(privateRoot.filename().string().starts_with("rn-tizen-"));
  CHECK((fs::status(privateRoot).permissions() & fs::perms::all) == fs::perms::owner_all);
  CHECK(fs::is_directory(directories.data));
  CHECK(fs::is_directory(directories.cache));
  CHECK(directories.resource.empty());
  CHECK(readAppDirectories().data == directories.data);
  fs::remove_all(privateRoot);
}

TEST_CASE("fileUri prefixes a path with the file scheme") {
  CHECK(fileUri("/opt/usr/data/") == "file:///opt/usr/data/");
}

} // namespace rntizen::expo
