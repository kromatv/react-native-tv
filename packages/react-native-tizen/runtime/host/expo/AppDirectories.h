#pragma once

#include <string>

namespace rntizen::expo {

/// Where the app may keep files, as absolute paths with a trailing slash.
/// `resource` is empty when the platform does not say.
struct AppDirectories {
  std::string data;
  std::string cache;
  std::string resource;
};

/// The app's directories from Tizen's app_common API, read once. The data and
/// cache directories exist when this returns.
const AppDirectories &appDirectories();

/// `path` as a `file://` URI.
std::string fileUri(const std::string &path);

} // namespace rntizen::expo
