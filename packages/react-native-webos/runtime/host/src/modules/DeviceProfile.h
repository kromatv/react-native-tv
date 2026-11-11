#pragma once

#include <cstdlib>
#include <functional>
#include <string>

namespace rnwebos {

/// What the TV says about itself, read once from webOS's system info.
struct DeviceProfile {
  std::string modelName;
  std::string platformVersion;
  std::string buildString;
  int screenWidth;
  int screenHeight;
};

/// Looks an environment variable up: its value, or null when it is not set.
using EnvironmentLookup = std::function<const char *(const char *name)>;

/// The TV as `environment` describes it (`RN_WEBOS_MODEL_NAME`,
/// `RN_WEBOS_PLATFORM_VERSION`, `RN_WEBOS_BUILD`), on a 1920x1080 screen
/// until the window says otherwise.
DeviceProfile readDeviceProfile(const EnvironmentLookup &environment = std::getenv);

} // namespace rnwebos
