#pragma once

#include <string>

namespace rnwebos::expo {

/// Where the app may keep files, as absolute paths with a trailing slash.
struct AppDirectories {
  std::string data;
  std::string cache;
  std::string resource;
};

/// The app's directories: the package's (`platform/AppPaths.h`) unless
/// `setAppDirectories` gave others. Any thread.
const AppDirectories &appDirectories();

/// Replaces what `appDirectories` answers, before anything asked for them: how
/// a test gives the Expo modules directories of its own.
void setAppDirectories(AppDirectories directories);

/// `path` as a `file://` URI.
std::string fileUri(const std::string &path);

} // namespace rnwebos::expo
