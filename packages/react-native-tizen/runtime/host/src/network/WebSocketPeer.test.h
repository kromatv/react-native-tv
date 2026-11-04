#pragma once

#include "LoopbackServer.test.h"
#include "WebSocketConnection.h"
#include "WebSocketHandshake.h"

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rntizen::testing {

inline constexpr std::size_t kLargeMessage = 8 * 1024 * 1024;

class WebSocketEvents final {
 public:
  WebSocketListener listener() {
    return {
        .onOpen = [this](std::string protocol) { add("open " + protocol); },
        .onMessage = [this](std::string data, bool binary) { add((binary ? "binary " : "text ") + data); },
        .onClosed = [this](int code, std::string reason) { add("closed " + std::to_string(code) + " " + reason); },
        .onFailed = [this](std::string message) { add("failed " + message); },
    };
  }

  std::vector<std::string> seen() {
    std::lock_guard lock(mutex_);
    return events_;
  }

  std::vector<std::string> waitFor(std::size_t count) {
    std::unique_lock lock(mutex_);
    changed_.wait_for(lock, kTimeout, [this, count] { return events_.size() >= count; });
    return events_;
  }

 private:
  static constexpr auto kTimeout = std::chrono::seconds(10);

  void add(std::string event) {
    std::lock_guard lock(mutex_);
    events_.push_back(std::move(event));
    changed_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<std::string> events_;
};

inline std::string headerValue(std::string_view request, std::string_view name) {
  const auto at = request.find(name);
  if (at == std::string_view::npos) return {};
  const auto start = at + name.size();
  return std::string(request.substr(start, request.find("\r\n", start) - start));
}

inline std::string readClientPayload(int socket) {
  const auto header = readBytes(socket, 2);
  const auto length = static_cast<std::uint8_t>(header[1]) & 0x7f;
  const auto mask = readBytes(socket, 4);
  auto payload = readBytes(socket, length);
  for (std::size_t index = 0; index < payload.size(); ++index) payload[index] ^= mask[index % 4];
  return payload;
}

inline std::string serverFrame(std::uint8_t first, std::string_view payload) {
  return std::string{static_cast<char>(first), static_cast<char>(payload.size())} + std::string(payload);
}

inline void acceptHandshake(int socket, std::string_view extraHeaders = "") {
  const auto request = readUntil(socket, "\r\n\r\n");
  const auto key = headerValue(request, "Sec-WebSocket-Key: ");
  writeAll(
      socket,
      "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
          webSocketAccept(key) + "\r\n" + std::string(extraHeaders) + "\r\n");
}

inline std::size_t readUntilClosed(int socket) {
  std::size_t total = 0;
  std::array<char, 64 * 1024> chunk{};
  while (true) {
    const auto got = ::recv(socket, chunk.data(), chunk.size(), 0);
    if (got <= 0) return total;
    total += static_cast<std::size_t>(got);
  }
}

inline void resetConnection(int socket) {
  const linger abortive{.l_onoff = 1, .l_linger = 0};
  setsockopt(socket, SOL_SOCKET, SO_LINGER, &abortive, sizeof abortive);
  ::close(socket);
}

inline WebSocketRequest requestTo(const LoopbackServer &server) {
  return {.url = server.url("ws"), .protocols = {}, .headers = {}};
}

} // namespace rntizen::testing
