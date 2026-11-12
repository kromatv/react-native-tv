#pragma once

#include <app_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Quits the Ecore main loop the launcher runs. Safe from any thread.
void ui_app_exit(void);

#ifdef __cplusplus
}
#endif
