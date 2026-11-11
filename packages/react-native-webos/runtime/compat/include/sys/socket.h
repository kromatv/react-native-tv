// <sys/socket.h> with glibc 2.14's sendmmsg, which the webOS SDK's glibc
// 2.12.2 does not declare. Defined in runtime/compat.

#pragma once

#include_next <sys/socket.h>

#ifdef __USE_GNU
#ifdef __cplusplus
extern "C" {
#endif

int sendmmsg(int socket, struct mmsghdr *messages, unsigned int count, int flags);

#ifdef __cplusplus
}
#endif
#endif
