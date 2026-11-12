#include "AppDirectories.h"

#include "ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <string>

namespace rnwebos::expo {

namespace {

namespace fs = std::filesystem;

std::string withSlash(const fs::path &path) {
  return path.string() + "/";
}

} // namespace

TEST_CASE("appDirectories answers the directories it was given, with a trailing slash, every time") {
  const auto &directories = appDirectories();
  CHECK(directories.data == withSlash(testing::testAppRoot() / "data"));
  CHECK(directories.cache == withSlash(testing::testAppRoot() / "cache"));
  CHECK(directories.resource == withSlash(testing::testAppRoot() / "res"));
  CHECK(fs::is_directory(directories.data));
  CHECK(fs::is_directory(directories.cache));
  CHECK(&appDirectories() == &directories);
}

TEST_CASE("fileUri prefixes a path with the file scheme") {
  CHECK(fileUri("/media/data/") == "file:///media/data/");
}

} // namespace rnwebos::expo
