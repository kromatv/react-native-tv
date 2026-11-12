#pragma once

#include <string>

namespace rnwebos {

/// Where the app's files are, each ending in a slash.
struct AppPaths {
  /// The package's `res` directory: the bundle, its assets, the runtime config. Read-only.
  std::string resource;
  /// What the app stores for good.
  std::string data;
  /// What the app can lose.
  std::string cache;
};

/// What the app's directories are worked out from.
struct AppPathSources {
  /// The directory the executable is in.
  std::string executableDirectory;
  /// `RN_WEBOS_RESOURCE_DIR`: the resources' directory, or empty for `res` beside the executable.
  std::string resourceOverride;
  /// `RN_WEBOS_APP_DATA_DIR`: where `data` and `cache` go, or empty for `appdata` beside the executable.
  std::string dataOverride;
};

/// The directories of an app started from `sources`, its `data` and `cache`
/// created: under a private directory of the system's temporary one when they
/// cannot be written where `sources` puts them.
AppPaths resolveAppPaths(const AppPathSources &sources);

/// The app's directories. A package ships a world-writable `appdata` beside
/// its executable for `data` and `cache`, since the TV runs the app as a user
/// that does not own its install directory
/// (docs/reference/webos-tv-platform.md, section 3.1); when that cannot be
/// written, both fall back to a private directory under the system's
/// temporary one. `RN_WEBOS_RESOURCE_DIR` and `RN_WEBOS_APP_DATA_DIR` override
/// the first two lookups. Created at the first call; any thread.
const AppPaths &appPaths();

} // namespace rnwebos
