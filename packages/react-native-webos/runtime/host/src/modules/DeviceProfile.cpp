#include "DeviceProfile.h"

#include <cstdlib>

namespace rnwebos {

namespace {

std::string environment(const char *name, const char *fallback) {
  const char *value = std::getenv(name);
  return value != nullptr && *value != '\0' ? value : fallback;
}

} // namespace

DeviceProfile readDeviceProfile() {
  return DeviceProfile{
      .modelName = environment("RN_WEBOS_MODEL_NAME", "LG TV"),
      .platformVersion = environment("RN_WEBOS_PLATFORM_VERSION", "unknown"),
      .buildString = environment("RN_WEBOS_BUILD", "unknown"),
      .screenWidth = 1920,
      .screenHeight = 1080,
  };
}

} // namespace rnwebos
