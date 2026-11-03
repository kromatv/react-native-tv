#include <dlog.h>

#include <stdio.h>
#include <unistd.h>

static char priorityLetter(log_priority prio) {
  static const char letters[DLOG_PRIO_MAX] = {'?', 'D', 'V', 'D', 'I', 'W', 'E', 'F', 'S'};
  return prio >= 0 && prio < DLOG_PRIO_MAX ? letters[prio] : '?';
}

int dlog_vprint(log_priority prio, const char *tag, const char *fmt, va_list ap) {
  if (fmt == NULL) return DLOG_ERROR_INVALID_PARAMETER;
  char message[4096];
  vsnprintf(message, sizeof message, fmt, ap);
  fprintf(stderr, "%c/%s(%5d): %s\n", priorityLetter(prio), tag != NULL ? tag : "", (int)getpid(), message);
  return DLOG_ERROR_NONE;
}

int dlog_print(log_priority prio, const char *tag, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int result = dlog_vprint(prio, tag, fmt, ap);
  va_end(ap);
  return result;
}
