// sendmmsg, glibc 2.14, which folly's NetOps.cpp requires of every Linux.

#define _GNU_SOURCE
#include "compat.h"

#include <errno.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

RN_WEBOS_COMPAT int sendmmsg(int socket, struct mmsghdr *messages, unsigned int count, int flags) {
  long sent = syscall(SYS_sendmmsg, socket, messages, count, flags);
  if (sent >= 0 || errno != ENOSYS) return (int)sent;
  // The call is Linux 3.0's: before it, one sendmsg for each message.
  unsigned int index = 0;
  for (; index < count; index++) {
    ssize_t length = sendmsg(socket, &messages[index].msg_hdr, flags);
    if (length < 0) return index > 0 ? (int)index : -1;
    messages[index].msg_len = (unsigned int)length;
  }
  return (int)index;
}
