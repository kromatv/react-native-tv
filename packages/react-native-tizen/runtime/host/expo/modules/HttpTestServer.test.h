#pragma once

#include "network/LoopbackServer.test.h"

#include <sys/time.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <format>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace rntizen::expo::testing {

/// An HTTP/1.1 response with a Content-Length and the given extra header lines.
inline std::string httpResponse(int status, std::string_view reason, std::string_view body, std::string_view headers = "") {
  return std::format(
      "HTTP/1.1 {} {}\r\nContent-Length: {}\r\nConnection: close\r\n{}\r\n{}", status, reason, body.size(), headers, body);
}

/// Answers each connection on the loopback interface, from a thread of its
/// own, with what `respond` returns for the request, then closes it. Keeps
/// every request it read, head and body; a client silent for five seconds
/// gets what was read so far, so the server always stops.
class HttpTestServer final {
 public:
  using Respond = std::function<std::string(const std::string &request)>;

  explicit HttpTestServer(Respond respond) : respond_(std::move(respond)) {}

  ~HttpTestServer() {
    stopping_ = true;
    thread_.join();
  }

  HttpTestServer(const HttpTestServer &) = delete;
  HttpTestServer &operator=(const HttpTestServer &) = delete;

  std::string url(std::string_view path) const {
    return server_.url("http", path);
  }

  std::vector<std::string> requests() const {
    std::lock_guard lock(mutex_);
    return requests_;
  }

 private:
  static std::size_t contentLength(const std::string &head) {
    std::string lower(head);
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const auto at = lower.find("content-length: ");
    return at == std::string::npos ? 0 : std::stoul(lower.substr(at + 16));
  }

  void serve() {
    while (!stopping_) {
      const int socket = server_.accept(std::chrono::milliseconds(20));
      if (socket < 0) continue;
      const timeval patience{.tv_sec = 5, .tv_usec = 0};
      setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &patience, sizeof patience);
      auto request = rntizen::testing::readUntil(socket, "\r\n\r\n");
      request += rntizen::testing::readBytes(socket, contentLength(request));
      {
        std::lock_guard lock(mutex_);
        requests_.push_back(request);
      }
      rntizen::testing::writeAll(socket, respond_(request));
      ::close(socket);
    }
  }

  rntizen::testing::LoopbackServer server_;
  Respond respond_;
  std::atomic<bool> stopping_{false};
  mutable std::mutex mutex_;
  std::vector<std::string> requests_;
  std::thread thread_{[this] { serve(); }};
};

} // namespace rntizen::expo::testing
