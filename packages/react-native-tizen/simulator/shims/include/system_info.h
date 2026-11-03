#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SYSTEM_INFO_ERROR_NONE = 0,
  SYSTEM_INFO_ERROR_INVALID_PARAMETER = -22,
  SYSTEM_INFO_ERROR_OUT_OF_MEMORY = -12,
  SYSTEM_INFO_ERROR_IO_ERROR = -5,
  SYSTEM_INFO_ERROR_PERMISSION_DENIED = -13,
  SYSTEM_INFO_ERROR_NOT_SUPPORTED = -1073741822,
} system_info_error_e;

/// The simulator answers as a 1920x1080 Tizen 9.0 TV named "Simulator", or at
/// `$RN_TIZEN_SIMULATOR_SCREEN_SIZE` (such as `1280x720`) when set; a key it
/// does not know is SYSTEM_INFO_ERROR_INVALID_PARAMETER, as on a TV.
int system_info_get_platform_int(const char *key, int *value);

/// `*value` is malloc'd; the caller frees it.
int system_info_get_platform_string(const char *key, char **value);

#ifdef __cplusplus
}
#endif
