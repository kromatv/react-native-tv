#include "AppDirectories.h"

#include "../src/platform/AppPaths.h"

namespace rnwebos::expo {

AppDirectories readAppDirectories() {
  const auto &paths = appPaths();
  return {.data = paths.data, .cache = paths.cache, .resource = paths.resource};
}

const AppDirectories &appDirectories() {
  static const AppDirectories directories = readAppDirectories();
  return directories;
}

std::string fileUri(const std::string &path) {
  return "file://" + path;
}

} // namespace rnwebos::expo
