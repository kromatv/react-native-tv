#include "AppPaths.h"

#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rnwebos {

namespace {

namespace fs = std::filesystem;

struct InstallDirectory {
  InstallDirectory() {
    fs::remove_all(path);
    fs::create_directories(path);
  }
  ~InstallDirectory() {
    fs::remove_all(path);
  }
  InstallDirectory(const InstallDirectory &) = delete;
  InstallDirectory &operator=(const InstallDirectory &) = delete;

  const fs::path path{fs::temp_directory_path() / ("rn-webos-app-paths-test-" + std::to_string(getpid()))};
};

std::string withSlash(const fs::path &path) {
  return path.string() + "/";
}

} // namespace

TEST_CASE_FIXTURE(InstallDirectory, "resolveAppPaths puts resources, data and cache beside the executable, each with a trailing slash") {
  const auto paths = resolveAppPaths({.executableDirectory = path.string(), .resourceOverride = "", .dataOverride = ""});

  CHECK(paths.resource == withSlash(path / "res"));
  CHECK(paths.data == withSlash(path / "appdata" / "data"));
  CHECK(paths.cache == withSlash(path / "appdata" / "cache"));
  CHECK(fs::is_directory(paths.data));
  CHECK(fs::is_directory(paths.cache));
  CHECK_FALSE(fs::exists(paths.resource));
}

TEST_CASE_FIXTURE(InstallDirectory, "resolveAppPaths takes the resource and the data directories it is told to") {
  const auto resources = path / "bundle" / "";
  const auto storage = path / "storage";

  const auto paths = resolveAppPaths(
      {.executableDirectory = (path / "bin").string(), .resourceOverride = resources.string(), .dataOverride = storage.string()});
  const auto unslashed = resolveAppPaths(
      {.executableDirectory = (path / "bin").string(), .resourceOverride = (path / "bundle").string(), .dataOverride = storage.string()});

  CHECK(paths.resource == withSlash(path / "bundle"));
  CHECK(unslashed.resource == paths.resource);
  CHECK(paths.data == withSlash(storage / "data"));
  CHECK(paths.cache == withSlash(storage / "cache"));
  CHECK(fs::is_directory(paths.cache));
  CHECK_FALSE(fs::exists(path / "bin"));
}

TEST_CASE_FIXTURE(InstallDirectory, "resolveAppPaths falls back to a private temporary directory when the app's own cannot be written") {
  const auto blocker = path / "not-a-directory";
  std::ofstream(blocker).close();

  const auto paths = resolveAppPaths({.executableDirectory = blocker.string(), .resourceOverride = "", .dataOverride = ""});
  const auto again = resolveAppPaths({.executableDirectory = path.string(), .resourceOverride = "", .dataOverride = blocker.string()});

  const fs::path data(paths.data);
  const auto privateRoot = data.parent_path().parent_path();
  CHECK(data.filename().empty());
  CHECK(data.parent_path().filename() == "data");
  CHECK(paths.cache == withSlash(privateRoot / "cache"));
  CHECK(fs::equivalent(privateRoot.parent_path(), fs::temp_directory_path()));
  CHECK(privateRoot.filename().string().starts_with("rn-webos-"));
  CHECK((fs::status(privateRoot).permissions() & fs::perms::all) == fs::perms::owner_all);
  CHECK(fs::is_directory(paths.data));
  CHECK(fs::is_directory(paths.cache));
  CHECK(paths.resource == withSlash(blocker / "res"));
  CHECK(again.data != paths.data);
  CHECK(fs::is_directory(again.data));
  fs::remove_all(privateRoot);
  fs::remove_all(fs::path(again.data).parent_path().parent_path());
}

TEST_CASE("appPaths answers the same directories every time") {
  const auto &paths = appPaths();

  CHECK(&appPaths() == &paths);
  CHECK(paths.resource.ends_with('/'));
  CHECK(fs::is_directory(paths.data));
  CHECK(fs::is_directory(paths.cache));
}

} // namespace rnwebos
