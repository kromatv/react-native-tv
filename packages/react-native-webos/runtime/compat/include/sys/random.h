// glibc 2.25's <sys/random.h>, which the webOS SDK's glibc 2.12.2 does not
// have. The functions are defined in runtime/compat.

#pragma once

#include <sys/types.h>

#define GRND_NONBLOCK 0x01
#define GRND_RANDOM 0x02

#ifdef __cplusplus
extern "C" {
#endif

ssize_t getrandom(void *buffer, size_t length, unsigned int flags);
int getentropy(void *buffer, size_t length);

#ifdef __cplusplus
}
#endif
