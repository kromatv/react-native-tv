#include "DeviceName.h"

#include <dlfcn.h>

#include <cstdlib>

#if __has_include(<system/system_settings.h>)
#include <system/system_settings.h>
#define RN_TIZEN_HAS_SYSTEM_SETTINGS 1
#endif

namespace rntizen::expo {

std::string tvDeviceName(const std::string &fallback) {
#ifdef RN_TIZEN_HAS_SYSTEM_SETTINGS
  // Opened at run time: the simulator has no stand-in for this library.
  static void *library = dlopen("libcapi-system-system-settings.so.0", RTLD_LAZY | RTLD_LOCAL);
  if (library == nullptr) return fallback;
  using GetString = int (*)(system_settings_key_e, char **);
  static auto getString = reinterpret_cast<GetString>(dlsym(library, "system_settings_get_value_string"));
  if (getString == nullptr) return fallback;
  char *value = nullptr;
  if (getString(SYSTEM_SETTINGS_KEY_DEVICE_NAME, &value) != SYSTEM_SETTINGS_ERROR_NONE || value == nullptr) {
    return fallback;
  }
  std::string name = value;
  std::free(value);
  return name.empty() ? fallback : name;
#else
  return fallback;
#endif
}

} // namespace rntizen::expo
