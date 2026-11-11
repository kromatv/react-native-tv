#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rntizen::testing {

class LoopbackServer final {
 public:
  explicit LoopbackServer(std::uint16_t port = 0) : socket_(::socket(AF_INET, SOCK_STREAM, 0)) {
    const int reuse = 1;
    setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(socket_, reinterpret_cast<const sockaddr *>(&address), sizeof address) != 0 || listen(socket_, 4) != 0) {
      throw std::runtime_error("cannot listen on the loopback interface");
    }
    socklen_t length = sizeof address;
    getsockname(socket_, reinterpret_cast<sockaddr *>(&address), &length);
    port_ = ntohs(address.sin_port);
  }

  ~LoopbackServer() { ::close(socket_); }

  LoopbackServer(const LoopbackServer &) = delete;
  LoopbackServer &operator=(const LoopbackServer &) = delete;

  std::uint16_t port() const { return port_; }

  std::string url(std::string_view scheme, std::string_view path = "/") const {
    return std::string(scheme) + "://127.0.0.1:" + std::to_string(port_) + std::string(path);
  }

  int accept(std::chrono::milliseconds timeout) const {
    pollfd waiting{.fd = socket_, .events = POLLIN, .revents = 0};
    if (poll(&waiting, 1, static_cast<int>(timeout.count())) != 1) return -1;
    return ::accept(socket_, nullptr, nullptr);
  }

 private:
  int socket_;
  std::uint16_t port_{0};
};

inline std::string readUntil(int socket, std::string_view end) {
  std::string received;
  char byte = 0;
  while (received.find(end) == std::string::npos && ::recv(socket, &byte, 1, 0) == 1) received.push_back(byte);
  return received;
}

inline std::string readBytes(int socket, std::size_t count) {
  std::string received(count, '\0');
  std::size_t done = 0;
  while (done < count) {
    const auto got = ::recv(socket, received.data() + done, count - done, 0);
    if (got <= 0) break;
    done += static_cast<std::size_t>(got);
  }
  received.resize(done);
  return received;
}

inline void writeAll(int socket, std::string_view bytes) {
  while (!bytes.empty()) {
    const auto sent = ::send(socket, bytes.data(), bytes.size(), MSG_NOSIGNAL);
    if (sent <= 0) return;
    bytes.remove_prefix(static_cast<std::size_t>(sent));
  }
}

} // namespace rntizen::testing
