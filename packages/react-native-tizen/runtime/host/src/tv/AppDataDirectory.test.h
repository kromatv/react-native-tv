#pragma once

#include <doctest/doctest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace rntizen::testing {

/// A fresh directory the simulator's app_common shim answers as the app's data
/// directory until the scope ends, then removed. `path()` is where the data
/// directory is, without creating it.
class AppDataDirectory final {
 public:
  AppDataDirectory() : root_(makeRoot()), path_(root_ / "data") {
    if (const char *previous = std::getenv(kVariable)) previous_ = previous;
    point(path_);
  }

  ~AppDataDirectory() {
    if (previous_) {
      setenv(kVariable, previous_->c_str(), 1);
    } else {
      unsetenv(kVariable);
    }
    std::error_code ignored;
    std::filesystem::remove_all(root_, ignored);
  }

  AppDataDirectory(const AppDataDirectory &) = delete;
  AppDataDirectory &operator=(const AppDataDirectory &) = delete;

  const std::filesystem::path &path() const {
    return path_;
  }

  /// Leaves the app without a data directory: the shim cannot create one under a file.
  void makeUnavailable() const {
    const auto blocker = root_ / "file";
    std::ofstream(blocker).close();
    point(blocker / "data");
  }

  std::string read(const char *name) const {
    std::ifstream file(path_ / name, std::ios::binary);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
  }

 private:
  static constexpr const char *kVariable = "RN_TIZEN_SIMULATOR_DATA_DIR";

  static std::filesystem::path makeRoot() {
    auto pattern = (std::filesystem::temp_directory_path() / "rn-tizen-tv-tests-XXXXXX").string();
    REQUIRE(mkdtemp(pattern.data()) != nullptr);
    return pattern;
  }

  static void point(const std::filesystem::path &path) {
    setenv(kVariable, path.c_str(), 1);
  }

  const std::filesystem::path root_;
  const std::filesystem::path path_;
  std::optional<std::string> previous_;
};

} // namespace rntizen::testing
