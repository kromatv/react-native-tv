#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rntizen {

/// Where a `ws://` or `wss://` URL points: the `http(s)://host:port/` libcurl
/// connects to, the `host[:port]` of the `Host` header (the port only when the
/// URL names one), the path and query to ask for, and the `Origin` React
/// Native's Android module sends when JavaScript sets none.
struct WebSocketEndpoint {
  std::string connectUrl;
  std::string authority;
  std::string target;
  std::string origin;
};

/// Reads `url` with libcurl's URL parser. `http` and `https` stand for `ws` and
/// `wss`, as on Android; throws std::invalid_argument for any other scheme.
WebSocketEndpoint parseWebSocketUrl(const std::string &url);

/// A fresh `Sec-WebSocket-Key`: 16 random bytes in base64 (RFC 6455 section 4.1).
std::string makeWebSocketKey();

/// The `Sec-WebSocket-Accept` a server must answer `key` with (RFC 6455 section 4.2.2).
std::string webSocketAccept(std::string_view key);

/// The opening handshake: a GET asking to upgrade, with JavaScript's headers
/// after the ones the protocol owns.
std::string buildWebSocketHandshake(
    const WebSocketEndpoint &endpoint,
    std::string_view key,
    const std::vector<std::string> &protocols,
    const std::vector<std::pair<std::string, std::string>> &headers);

/// Checks the server's answer, its status line and headers up to the blank
/// line, as RFC 6455 section 4.1 tells a client to, and returns the protocol
/// the server picked (empty when none). Throws std::runtime_error naming what
/// is wrong.
std::string checkWebSocketHandshake(
    std::string_view head,
    std::string_view key,
    const std::vector<std::string> &protocols);

} // namespace rntizen
