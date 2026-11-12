#pragma once

#include <string>

namespace rntizen {

/// What the TV says about itself, read once from Tizen's system info.
struct DeviceProfile {
  std::string modelName;
  std::string platformVersion;
  std::string buildString;
  int screenWidth;
  int screenHeight;
};

DeviceProfile readDeviceProfile();

} // namespace rntizen
