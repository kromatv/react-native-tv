#pragma once

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  DLOG_ERROR_NONE = 0,
  DLOG_ERROR_INVALID_PARAMETER = -22,
  DLOG_ERROR_NOT_PERMITTED = -1,
} dlog_error_e;

typedef enum {
  DLOG_UNKNOWN = 0,
  DLOG_DEFAULT,
  DLOG_VERBOSE,
  DLOG_DEBUG,
  DLOG_INFO,
  DLOG_WARN,
  DLOG_ERROR,
  DLOG_FATAL,
  DLOG_SILENT,
  DLOG_PRIO_MAX,
} log_priority;

/// Writes one line to stderr in dlogutil's `I/TAG( pid): message` shape.
int dlog_print(log_priority prio, const char *tag, const char *fmt, ...) __attribute__((format(printf, 3, 4)));

int dlog_vprint(log_priority prio, const char *tag, const char *fmt, va_list ap);

#ifdef __cplusplus
}
#endif
