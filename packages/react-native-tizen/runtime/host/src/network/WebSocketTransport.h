#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// A TCP or TLS connection libcurl opened and handed over
/// (`CURLOPT_CONNECT_ONLY`), read and written without blocking. One thread
/// only.
class WebSocketTransport final {
 public:
  /// Connects to `url` (`http://` or `https://`, verified as libcurl verifies
  /// any request) and returns once connected. Gives up when `abort` turns
  /// true, within about a second. Throws std::runtime_error on failure.
  WebSocketTransport(const std::string &url, const std::atomic<bool> &abort);
  ~WebSocketTransport();

  WebSocketTransport(const WebSocketTransport &) = delete;
  WebSocketTransport &operator=(const WebSocketTransport &) = delete;

  /// The socket to poll.
  int socket() const;

  /// Reads what has arrived into `buffer`: the byte count, 0 once the peer
  /// closed the connection, or nullopt when nothing is waiting. Throws
  /// std::runtime_error on a broken connection.
  std::optional<std::size_t> receive(char *buffer, std::size_t capacity);

  /// Writes as much of `bytes` as the socket takes now and returns how much.
  /// Throws std::runtime_error on a broken connection.
  std::size_t send(std::string_view bytes);

 private:
  void *easy_;
  int socket_{-1};
};

} // namespace rntizen
