// getauxval, glibc 2.16, for the CPU probe of the zlib inside Skia (CRC32
// instructions on the ARMv8 cores of newer TVs).

#define _GNU_SOURCE
#include "compat.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <pthread.h>
#include <sys/auxv.h>
#include <unistd.h>

#define MAX_ENTRIES 64

static unsigned long (*libc_getauxval)(unsigned long);
static ElfW(auxv_t) entries[MAX_ENTRIES];
static pthread_once_t once = PTHREAD_ONCE_INIT;

// A TV from webOS 2 on has the function in its own glibc, which this hidden
// definition does not shadow for dlsym. Before that, the kernel's copy of the
// vector is read, as the SDK's own polyfill does.
static void load(void) {
  libc_getauxval = (unsigned long (*)(unsigned long))dlsym(RTLD_NEXT, "getauxval");
  if (libc_getauxval != NULL) return;
  int file = open("/proc/self/auxv", O_RDONLY | O_CLOEXEC);
  if (file < 0) return;
  size_t filled = 0;
  while (filled < sizeof entries - sizeof entries[0]) {
    ssize_t received = read(file, (char *)entries + filled, sizeof entries - sizeof entries[0] - filled);
    if (received < 0 && errno == EINTR) continue;
    if (received <= 0) break;
    filled += (size_t)received;
  }
  close(file);
}

RN_WEBOS_COMPAT unsigned long getauxval(unsigned long type) {
  pthread_once(&once, load);
  if (libc_getauxval != NULL) return libc_getauxval(type);
  for (const ElfW(auxv_t) *entry = entries; entry->a_type != AT_NULL; entry++) {
    if (entry->a_type == type) return entry->a_un.a_val;
  }
  errno = ENOENT;
  return 0;
}
