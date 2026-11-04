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

/// The app's directories as Tizen's app_common API reports them now, creating
/// the data and cache directories. A path it does not report falls back to a
/// private directory of the process's own.
AppDirectories readAppDirectories();

/// readAppDirectories(), read once.
const AppDirectories &appDirectories();

/// `path` as a `file://` URI.
std::string fileUri(const std::string &path);

} // namespace rntizen::expo
