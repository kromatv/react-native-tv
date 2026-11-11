// Mbed TLS's entropy source (MBEDTLS_ENTROPY_HARDWARE_ALT): the kernel's
// generator through getrandom, which on a TV is runtime/compat's.

#include <mbedtls/entropy.h>

#include "entropy_poll.h"

#include <errno.h>
#include <sys/random.h>

int mbedtls_hardware_poll(void *data, unsigned char *output, size_t length, size_t *written) {
  (void)data;
  ssize_t received;
  do {
    received = getrandom(output, length, 0);
  } while (received < 0 && errno == EINTR);
  if (received < 0) return MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
  *written = (size_t)received;
  return 0;
}
