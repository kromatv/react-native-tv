#include "WebSocketFrame.h"

#include <stdexcept>

namespace rntizen {

namespace {

constexpr std::uint8_t kFinalBit = 0x80;
constexpr std::uint8_t kReservedBits = 0x70;
constexpr std::uint8_t kMaskBit = 0x80;
constexpr std::uint8_t kLengthBits = 0x7f;
constexpr std::uint64_t kMaxControlPayload = 125;

bool isControl(std::uint8_t opcode) {
  return (opcode & 0x08) != 0;
}

bool isKnown(std::uint8_t opcode) {
  return opcode <= 0x2 || (opcode >= 0x8 && opcode <= 0xA);
}

std::uint8_t byteAt(const std::string &buffer, std::size_t index) {
  return static_cast<std::uint8_t>(buffer[index]);
}

} // namespace

std::string encodeWebSocketFrame(WebSocketOpcode opcode, std::string_view payload, std::array<std::uint8_t, 4> mask) {
  std::string frame;
  frame.reserve(payload.size() + 14);
  frame.push_back(static_cast<char>(kFinalBit | static_cast<std::uint8_t>(opcode)));
  const std::uint64_t size = payload.size();
  if (size < 126) {
    frame.push_back(static_cast<char>(kMaskBit | size));
  } else if (size <= 0xffff) {
    frame.push_back(static_cast<char>(kMaskBit | 126));
    frame.push_back(static_cast<char>(size >> 8));
    frame.push_back(static_cast<char>(size));
  } else {
    frame.push_back(static_cast<char>(kMaskBit | 127));
    for (int shift = 56; shift >= 0; shift -= 8) frame.push_back(static_cast<char>(size >> shift));
  }
  for (auto byte : mask) frame.push_back(static_cast<char>(byte));
  for (std::size_t index = 0; index < payload.size(); ++index) {
    frame.push_back(static_cast<char>(static_cast<std::uint8_t>(payload[index]) ^ mask[index % 4]));
  }
  return frame;
}

std::optional<WebSocketFrame> takeWebSocketFrame(std::string &buffer, std::uint64_t maxPayload) {
  if (buffer.size() < 2) return std::nullopt;
  const auto first = byteAt(buffer, 0);
  const auto second = byteAt(buffer, 1);
  const std::uint8_t opcode = first & 0x0f;
  if ((first & kReservedBits) != 0) throw std::runtime_error("The server set reserved bits in a frame");
  if (!isKnown(opcode)) throw std::runtime_error("The server sent an unknown opcode " + std::to_string(opcode));
  if ((second & kMaskBit) != 0) throw std::runtime_error("The server sent a masked frame");
  std::uint64_t size = second & kLengthBits;
  std::size_t header = 2;
  if (size == 126) {
    if (buffer.size() < 4) return std::nullopt;
    size = (static_cast<std::uint64_t>(byteAt(buffer, 2)) << 8) | byteAt(buffer, 3);
    header = 4;
  } else if (size == 127) {
    if (buffer.size() < 10) return std::nullopt;
    size = 0;
    for (std::size_t index = 2; index < 10; ++index) size = (size << 8) | byteAt(buffer, index);
    header = 10;
  }
  const bool final = (first & kFinalBit) != 0;
  if (isControl(opcode) && (!final || size > kMaxControlPayload)) {
    throw std::runtime_error("The server sent a fragmented or oversized control frame");
  }
  if (size > maxPayload) throw std::runtime_error("The server sent a frame of " + std::to_string(size) + " bytes");
  if (buffer.size() - header < size) return std::nullopt;
  WebSocketFrame frame{
      .final = final,
      .opcode = static_cast<WebSocketOpcode>(opcode),
      .payload = buffer.substr(header, static_cast<std::size_t>(size)),
  };
  buffer.erase(0, header + static_cast<std::size_t>(size));
  return frame;
}

std::string encodeWebSocketClose(int code, std::string_view reason) {
  std::string payload;
  payload.push_back(static_cast<char>((code >> 8) & 0xff));
  payload.push_back(static_cast<char>(code & 0xff));
  payload.append(reason.substr(0, kMaxControlPayload - 2));
  return payload;
}

std::pair<int, std::string> decodeWebSocketClose(std::string_view payload) {
  if (payload.size() < 2) return {1005, {}};
  const int code = (static_cast<std::uint8_t>(payload[0]) << 8) | static_cast<std::uint8_t>(payload[1]);
  return {code, std::string(payload.substr(2))};
}

} // namespace rntizen
