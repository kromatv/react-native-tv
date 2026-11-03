#include "TcpConnection.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>

namespace rntizen {

namespace {

constexpr int kPollSliceMs = 250;

bool setBlocking(int socket) {
  const int flags = fcntl(socket, F_GETFL);
  return flags >= 0 && fcntl(socket, F_SETFL, flags & ~O_NONBLOCK) == 0;
}

bool awaitConnected(int socket, std::chrono::milliseconds timeout, const std::function<bool()> &cancelled) {
  for (int waited = 0; waited < timeout.count() && !cancelled(); waited += kPollSliceMs) {
    pollfd target{.fd = socket, .events = POLLOUT, .revents = 0};
    const int ready = poll(&target, 1, kPollSliceMs);
    if (ready < 0 && errno != EINTR) return false;
    if (ready <= 0) continue;
    int error = 0;
    socklen_t length = sizeof error;
    return getsockopt(socket, SOL_SOCKET, SO_ERROR, &error, &length) == 0 && error == 0;
  }
  return false;
}

} // namespace

int connectTcp(
    const std::string &host,
    std::uint16_t port,
    std::chrono::milliseconds timeout,
    const std::function<bool()> &cancelled) {
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1) return -1;
  const int candidate = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
  if (candidate < 0) return -1;
  const bool connected = connect(candidate, reinterpret_cast<sockaddr *>(&address), sizeof address) == 0 ||
                         (errno == EINPROGRESS && awaitConnected(candidate, timeout, cancelled));
  if (connected && setBlocking(candidate)) return candidate;
  close(candidate);
  return -1;
}

bool peerHasClosed(int socket) {
  pollfd target{.fd = socket, .events = POLLIN, .revents = 0};
  if (poll(&target, 1, 0) <= 0) return false;
  if ((target.revents & (POLLERR | POLLHUP)) != 0) return true;
  char byte = 0;
  const ssize_t peeked = recv(socket, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
  return peeked == 0 || (peeked < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR);
}

bool sendAll(int socket, const void *data, std::size_t size) {
  const auto *bytes = static_cast<const char *>(data);
  std::size_t sent = 0;
  while (sent < size) {
    const ssize_t written = send(socket, bytes + sent, size - sent, MSG_NOSIGNAL);
    if (written < 0 && errno == EINTR) continue;
    if (written <= 0) return false;
    sent += static_cast<std::size_t>(written);
  }
  return true;
}

} // namespace rntizen
