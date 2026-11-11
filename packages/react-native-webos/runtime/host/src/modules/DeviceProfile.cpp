#include "DeviceProfile.h"

namespace rnwebos {

namespace {

std::string valueOr(const EnvironmentLookup &environment, const char *name, const char *fallback) {
  const char *value = environment(name);
  return value != nullptr && *value != '\0' ? value : fallback;
}

} // namespace

DeviceProfile readDeviceProfile(const EnvironmentLookup &environment) {
  return DeviceProfile{
      .modelName = valueOr(environment, "RN_WEBOS_MODEL_NAME", "LG TV"),
      .platformVersion = valueOr(environment, "RN_WEBOS_PLATFORM_VERSION", "unknown"),
      .buildString = valueOr(environment, "RN_WEBOS_BUILD", "unknown"),
      .screenWidth = 1920,
      .screenHeight = 1080,
  };
}

} // namespace rnwebos
