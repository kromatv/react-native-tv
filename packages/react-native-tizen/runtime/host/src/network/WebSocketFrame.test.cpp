#include "NetworkError.h"
#include "WebSocketFrame.h"

#include <doctest/doctest.h>

#include <array>
#include <cstdint>
#include <string>

namespace rntizen {

namespace {

constexpr std::array<std::uint8_t, 4> kMask{0x37, 0xfa, 0x21, 0x3d};

std::string serverFrame(std::uint8_t first, std::string_view payload) {
  std::string frame;
  frame.push_back(static_cast<char>(first));
  frame.push_back(static_cast<char>(payload.size()));
  frame.append(payload);
  return frame;
}

std::string unmasked(std::string_view frame, std::size_t header) {
  std::string payload(frame.substr(header + 4));
  for (std::size_t index = 0; index < payload.size(); ++index) {
    payload[index] = static_cast<char>(static_cast<std::uint8_t>(payload[index]) ^ kMask[index % 4]);
  }
  return payload;
}

} // namespace

TEST_CASE("encodeWebSocketFrame masks a short text frame as RFC 6455 section 5.7 shows") {
  const auto frame = encodeWebSocketFrame(WebSocketOpcode::Text, "Hello", kMask);

  CHECK(frame == std::string("\x81\x85\x37\xfa\x21\x3d\x7f\x9f\x4d\x51\x58", 11));
}

TEST_CASE("encodeWebSocketFrame writes 16-bit and 64-bit lengths") {
  const std::string medium(300, 'm');
  const std::string large(70000, 'l');

  const auto mediumFrame = encodeWebSocketFrame(WebSocketOpcode::Binary, medium, kMask);
  const auto largeFrame = encodeWebSocketFrame(WebSocketOpcode::Binary, large, kMask);

  CHECK(static_cast<std::uint8_t>(mediumFrame[1]) == (0x80 | 126));
  CHECK(static_cast<std::uint8_t>(mediumFrame[2]) == 0x01);
  CHECK(static_cast<std::uint8_t>(mediumFrame[3]) == 0x2c);
  CHECK(unmasked(mediumFrame, 4) == medium);
  CHECK(static_cast<std::uint8_t>(largeFrame[1]) == (0x80 | 127));
  CHECK(largeFrame.substr(2, 8) == std::string("\0\0\0\0\0\x01\x11\x70", 8));
  CHECK(unmasked(largeFrame, 10) == large);
}

TEST_CASE("takeWebSocketFrame takes one complete frame at a time and waits for the rest") {
  std::string buffer = serverFrame(0x81, "one") + serverFrame(0x01, "par");
  buffer += std::string("\x80\x02", 2);

  const auto first = takeWebSocketFrame(buffer, 1024);
  const auto second = takeWebSocketFrame(buffer, 1024);
  const auto incomplete = takeWebSocketFrame(buffer, 1024);

  REQUIRE(first.has_value());
  CHECK(first->isFinal);
  CHECK(first->opcode == WebSocketOpcode::Text);
  CHECK(first->payload == "one");
  REQUIRE(second.has_value());
  CHECK_FALSE(second->isFinal);
  CHECK_FALSE(incomplete.has_value());
  buffer += "t2";
  CHECK(takeWebSocketFrame(buffer, 1024)->payload == "t2");
  CHECK(buffer.empty());
}

TEST_CASE("takeWebSocketFrame reads extended lengths") {
  const std::string payload(200, 'x');
  std::string buffer = std::string("\x82\x7e\x00\xc8", 4) + payload;
  std::string partial("\x82\x7f\x00\x00", 4);

  CHECK(takeWebSocketFrame(buffer, 1024)->payload == payload);
  CHECK_FALSE(takeWebSocketFrame(partial, 1024).has_value());
}

TEST_CASE("takeWebSocketFrame fails on frames a client must fail the connection over") {
  std::string reserved = serverFrame(0xc1, "x");
  std::string unknown = serverFrame(0x83, "x");
  std::string masked = std::string("\x81\x81\x00\x00\x00\x00x", 7);
  std::string fragmentedPing = serverFrame(0x09, "x");
  std::string longClose = std::string("\x88\x7e\x00\x7e", 4) + std::string(126, 'c');
  std::string tooLong = serverFrame(0x82, std::string(100, 'x'));

  CHECK_THROWS_AS(takeWebSocketFrame(reserved, 1024), NetworkError);
  CHECK_THROWS_AS(takeWebSocketFrame(unknown, 1024), NetworkError);
  CHECK_THROWS_AS(takeWebSocketFrame(masked, 1024), NetworkError);
  CHECK_THROWS_AS(takeWebSocketFrame(fragmentedPing, 1024), NetworkError);
  CHECK_THROWS_AS(takeWebSocketFrame(longClose, 1024), NetworkError);
  CHECK_THROWS_WITH_AS(takeWebSocketFrame(tooLong, 10), "The server sent a frame of 100 bytes", NetworkError);
}

TEST_CASE("a Close payload carries its code big-endian, then a reason of at most 123 bytes") {
  const auto payload = encodeWebSocketClose(4001, "bye");
  const auto truncated = encodeWebSocketClose(1000, std::string(200, 'r'));

  CHECK(payload == std::string("\x0f\xa1" "bye", 5));
  CHECK(decodeWebSocketClose(payload) == std::pair<int, std::string>{4001, "bye"});
  CHECK(truncated.size() == 125);
  CHECK(decodeWebSocketClose("").first == 1005);
}

} // namespace rntizen
