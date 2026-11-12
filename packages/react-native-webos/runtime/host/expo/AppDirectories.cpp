#include "AppDirectories.h"

#include "../src/platform/AppPaths.h"

#include <mutex>
#include <optional>
#include <utility>

namespace rnwebos::expo {

namespace {

struct Held {
  std::mutex mutex;
  std::optional<AppDirectories> directories;
};

Held &held() {
  static Held shared;
  return shared;
}

} // namespace

const AppDirectories &appDirectories() {
  auto &shared = held();
  std::lock_guard lock(shared.mutex);
  if (!shared.directories) {
    const auto &paths = appPaths();
    shared.directories = AppDirectories{.data = paths.data, .cache = paths.cache, .resource = paths.resource};
  }
  return *shared.directories;
}

void setAppDirectories(AppDirectories directories) {
  auto &shared = held();
  std::lock_guard lock(shared.mutex);
  shared.directories = std::move(directories);
}

std::string fileUri(const std::string &path) {
  return "file://" + path;
}

} // namespace rnwebos::expo
