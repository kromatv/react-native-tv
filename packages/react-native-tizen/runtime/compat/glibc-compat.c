// What GCC 14's static libstdc++ calls and the glibc 2.30 of a Tizen 9 TV does
// not export. Hidden, so a newer glibc's own definition never competes.

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/random.h>
#include <sys/types.h>

#define RN_TIZEN_HIDDEN __attribute__((visibility("hidden")))

// Zero tells libstdc++ the process may have threads, so it never skips an atomic.
RN_TIZEN_HIDDEN char __libc_single_threaded = 0;

RN_TIZEN_HIDDEN unsigned long __isoc23_strtoul(const char *text, char **end, int base) {
  return strtoul(text, end, base);
}

RN_TIZEN_HIDDEN uint32_t arc4random(void) {
  uint32_t value;
  for (;;) {
    ssize_t received = getrandom(&value, sizeof value, 0);
    if (received == (ssize_t)sizeof value) return value;
    if (received < 0 && errno != EINTR) abort();
  }
}
