#pragma once

/* Declared from the Tizen native API reference, Dlog:
 * https://samsungtizenos.com/docs/application/native/api/10.0.0/common/group__CAPI__SYSTEM__DLOG
 * The reference gives no enumerator values; these are the simulator's own. */

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DLOG_ERROR_NONE = 0,
  DLOG_ERROR_INVALID_PARAMETER,
  DLOG_ERROR_NOT_PERMITTED,
} dlog_error_e;

typedef enum {
  DLOG_VERBOSE,
  DLOG_DEBUG,
  DLOG_INFO,
  DLOG_WARN,
  DLOG_ERROR,
  DLOG_FATAL,
  DLOG_SILENT,
} log_priority;

/// Writes one line to stderr in dlogutil's `I/TAG( pid): message` shape.
int dlog_print(log_priority prio, const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

int dlog_vprint(log_priority prio, const char *tag, const char *fmt, va_list ap) __attribute__((format(printf, 3, 0)));

#ifdef __cplusplus
}
#endif
