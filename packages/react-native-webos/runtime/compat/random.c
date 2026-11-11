// getrandom and getentropy, glibc 2.25, for Mbed TLS's entropy source
// (runtime/mbedtls/entropy.c) and anything that seeds itself.

#define _GNU_SOURCE
#include "compat.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/random.h>
#include <sys/syscall.h>
#include <unistd.h>

// arch/arm/tools/syscall.tbl. The SDK's kernel headers are Linux 3.10's and
// the call is Linux 3.17's, so they do not number it.
#ifndef SYS_getrandom
#define SYS_getrandom 384
#endif

// Before Linux 3.17, as on a webOS 3 TV's 3.10, the call does not exist.
static ssize_t read_urandom(void *buffer, size_t length) {
  int file;
  do {
    file = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
  } while (file < 0 && errno == EINTR);
  if (file < 0) return -1;
  ssize_t received;
  do {
    received = read(file, buffer, length);
  } while (received < 0 && errno == EINTR);
  int error = errno;
  close(file);
  errno = error;
  return received;
}

RN_WEBOS_COMPAT ssize_t getrandom(void *buffer, size_t length, unsigned int flags) {
  long received = syscall(SYS_getrandom, buffer, length, flags);
  if (received < 0 && errno == ENOSYS) return read_urandom(buffer, length);
  return received;
}

RN_WEBOS_COMPAT int getentropy(void *buffer, size_t length) {
  if (length > 256) {
    errno = EIO;
    return -1;
  }
  unsigned char *bytes = buffer;
  while (length > 0) {
    ssize_t received = getrandom(bytes, length, 0);
    if (received < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    bytes += received;
    length -= (size_t)received;
  }
  return 0;
}
