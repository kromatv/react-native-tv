#include "DeviceProfile.h"

#include <system_info.h>

#include <cstdlib>

namespace rntizen {

namespace {

// A TV composes its UI plane at 1080p whatever the panel, so a missing value
// means 1920x1080 rather than the panel's 4K.
constexpr int kDefaultScreenWidth = 1920;
constexpr int kDefaultScreenHeight = 1080;

std::string platformString(const char *key, const char *fallback) {
  char *value = nullptr;
  if (system_info_get_platform_string(key, &value) != SYSTEM_INFO_ERROR_NONE || value == nullptr) return fallback;
  std::string result(value);
  std::free(value);
  return result;
}

int platformInt(const char *key, int fallback) {
  int value = 0;
  if (system_info_get_platform_int(key, &value) != SYSTEM_INFO_ERROR_NONE || value <= 0) return fallback;
  return value;
}

} // namespace

DeviceProfile readDeviceProfile() {
  return DeviceProfile{
      .modelName = platformString("http://tizen.org/system/model_name", "Samsung TV"),
      .platformVersion = platformString("http://tizen.org/feature/platform.version", "unknown"),
      .buildString = platformString("http://tizen.org/system/build.string", "unknown"),
      .screenWidth = platformInt("http://tizen.org/feature/screen.width", kDefaultScreenWidth),
      .screenHeight = platformInt("http://tizen.org/feature/screen.height", kDefaultScreenHeight),
  };
}

} // namespace rntizen
