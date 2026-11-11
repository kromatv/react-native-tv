#pragma once

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

DeviceProfile readDeviceProfile();

} // namespace rnwebos
