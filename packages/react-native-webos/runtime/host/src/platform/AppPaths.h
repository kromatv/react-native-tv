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

/// The app's directories. A package ships a world-writable `appdata` beside
/// its executable for `data` and `cache`, since the TV runs the app as a user
/// that does not own its install directory
/// (docs/reference/webos-tv-platform.md, section 3.1); when that cannot be
/// written, both fall back to a private directory under the system's
/// temporary one. `RN_WEBOS_RESOURCE_DIR` and `RN_WEBOS_APP_DATA_DIR` override
/// the first two lookups. Created at the first call; any thread.
const AppPaths &appPaths();

} // namespace rnwebos
