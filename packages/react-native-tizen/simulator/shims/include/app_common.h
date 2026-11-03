#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// `$RN_TIZEN_SIMULATOR_DATA_DIR`, else `$TMPDIR/rn-tizen-simulator/data`,
/// created if missing and ending in `/` like a TV's. The caller frees it.
char *app_get_data_path(void);

/// `$RN_TIZEN_SIMULATOR_CACHE_DIR`, else `$TMPDIR/rn-tizen-simulator/cache`,
/// created if missing and ending in `/` like a TV's. The caller frees it.
char *app_get_cache_path(void);

/// `$RN_TIZEN_SIMULATOR_RESOURCE_DIR`, the staged app's `res` directory, ending
/// in `/`; null when it is unset or missing. The caller frees it.
char *app_get_resource_path(void);

#ifdef __cplusplus
}
#endif
