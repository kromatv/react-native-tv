#include "AppPaths.h"

#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace rnwebos {

namespace fs = std::filesystem;

namespace {

std::string withTrailingSlash(std::string path) {
  if (!path.empty() && path.back() != '/') path += '/';
  return path;
}

std::string environment(const char *name) {
  const char *value = std::getenv(name);
  return value != nullptr ? value : "";
}

fs::path executableDirectory() {
  std::error_code error;
  const auto executable = fs::read_symlink("/proc/self/exe", error);
  return error ? fs::current_path(error) : executable.parent_path();
}

bool makeWritable(const fs::path &directory) {
  std::error_code error;
  fs::create_directories(directory, error);
  return !error && access(directory.c_str(), W_OK | X_OK) == 0;
}

fs::path temporaryRoot() {
  std::error_code error;
  auto pattern = (fs::temp_directory_path(error) / "rn-webos-XXXXXX").string();
  if (error || mkdtemp(pattern.data()) == nullptr) return {};
  return pattern;
}

AppPaths resolve() {
  const auto app = executableDirectory();
  const auto resource = environment("RN_WEBOS_RESOURCE_DIR");
  const auto dataOverride = environment("RN_WEBOS_APP_DATA_DIR");
  auto root = dataOverride.empty() ? app / "appdata" : fs::path(dataOverride);
  if (!makeWritable(root / "data") || !makeWritable(root / "cache")) {
    root = temporaryRoot();
    makeWritable(root / "data");
    makeWritable(root / "cache");
  }
  return {
      .resource = withTrailingSlash(resource.empty() ? (app / "res").string() : resource),
      .data = withTrailingSlash((root / "data").string()),
      .cache = withTrailingSlash((root / "cache").string()),
  };
}

} // namespace

const AppPaths &appPaths() {
  static const AppPaths paths = resolve();
  return paths;
}

} // namespace rnwebos
