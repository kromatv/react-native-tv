#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// A frame's opcode, RFC 6455 section 5.2.
enum class WebSocketOpcode : std::uint8_t {
  Continuation = 0x0,
  Text = 0x1,
  Binary = 0x2,
  Close = 0x8,
  Ping = 0x9,
  Pong = 0xA,
};

/// One frame a server sent.
struct WebSocketFrame {
  bool final;
  WebSocketOpcode opcode;
  std::string payload;
};

/// A client frame carrying the whole of `payload`, masked with `mask` as RFC
/// 6455 section 5.3 requires of every frame a client sends.
std::string encodeWebSocketFrame(WebSocketOpcode opcode, std::string_view payload, std::array<std::uint8_t, 4> mask);

/// Removes the first complete frame from the front of `buffer` and returns it,
/// or nullopt while the frame is still incomplete. Throws std::runtime_error
/// for a frame RFC 6455 tells a client to fail the connection over (a masked
/// frame, reserved bits, an unknown opcode, a fragmented or long control
/// frame) and for a payload above `maxPayload` bytes.
std::optional<WebSocketFrame> takeWebSocketFrame(std::string &buffer, std::uint64_t maxPayload);

/// The payload of a Close frame: the status code, big-endian, then the reason.
std::string encodeWebSocketClose(int code, std::string_view reason);

/// The status code and reason a Close payload carries; 1005 ("no status
/// received", RFC 6455 section 7.1.5) when it is empty.
std::pair<int, std::string> decodeWebSocketClose(std::string_view payload);

} // namespace rntizen
