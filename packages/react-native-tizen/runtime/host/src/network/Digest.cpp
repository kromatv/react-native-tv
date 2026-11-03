#include "Digest.h"

#include <cstring>
#include <vector>

namespace rntizen {

namespace {

constexpr std::array<std::uint32_t, 64> kMd5Constants = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};

constexpr std::array<int, 16> kMd5Shifts = {7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21};

std::uint32_t rotateLeft(std::uint32_t value, int count) {
  return (value << count) | (value >> (32 - count));
}

std::vector<std::uint8_t> padded(std::string_view data, bool bigEndianLength) {
  std::vector<std::uint8_t> message(data.begin(), data.end());
  const std::uint64_t bits = static_cast<std::uint64_t>(data.size()) * 8;
  message.push_back(0x80);
  while (message.size() % 64 != 56) message.push_back(0);
  for (int index = 0; index < 8; ++index) {
    const int shift = bigEndianLength ? 56 - index * 8 : index * 8;
    message.push_back(static_cast<std::uint8_t>(bits >> shift));
  }
  return message;
}

} // namespace

std::string md5Hex(std::string_view data) {
  std::uint32_t state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
  const auto message = padded(data, false);
  for (std::size_t offset = 0; offset < message.size(); offset += 64) {
    std::uint32_t words[16];
    for (int index = 0; index < 16; ++index) {
      const auto *bytes = &message[offset + index * 4];
      words[index] = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
    }
    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    for (int round = 0; round < 64; ++round) {
      std::uint32_t mixed;
      int word;
      if (round < 16) {
        mixed = (b & c) | (~b & d);
        word = round;
      } else if (round < 32) {
        mixed = (d & b) | (~d & c);
        word = (5 * round + 1) % 16;
      } else if (round < 48) {
        mixed = b ^ c ^ d;
        word = (3 * round + 5) % 16;
      } else {
        mixed = c ^ (b | ~d);
        word = (7 * round) % 16;
      }
      const auto next = d;
      d = c;
      c = b;
      b += rotateLeft(a + mixed + kMd5Constants[round] + words[word], kMd5Shifts[(round / 16) * 4 + round % 4]);
      a = next;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
  }
  static constexpr char kHex[] = "0123456789abcdef";
  std::string hex;
  for (auto word : state) {
    for (int index = 0; index < 4; ++index) {
      const auto byte = static_cast<std::uint8_t>(word >> (index * 8));
      hex += kHex[byte >> 4];
      hex += kHex[byte & 0x0f];
    }
  }
  return hex;
}

std::array<std::uint8_t, 20> sha1(std::string_view data) {
  std::uint32_t state[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  const auto message = padded(data, true);
  for (std::size_t offset = 0; offset < message.size(); offset += 64) {
    std::uint32_t words[80];
    for (int index = 0; index < 16; ++index) {
      const auto *bytes = &message[offset + index * 4];
      words[index] = (static_cast<std::uint32_t>(bytes[0]) << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    }
    for (int index = 16; index < 80; ++index) {
      words[index] = rotateLeft(words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16], 1);
    }
    std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];
    for (int round = 0; round < 80; ++round) {
      std::uint32_t mixed, constant;
      if (round < 20) {
        mixed = (b & c) | (~b & d);
        constant = 0x5a827999;
      } else if (round < 40) {
        mixed = b ^ c ^ d;
        constant = 0x6ed9eba1;
      } else if (round < 60) {
        mixed = (b & c) | (b & d) | (c & d);
        constant = 0x8f1bbcdc;
      } else {
        mixed = b ^ c ^ d;
        constant = 0xca62c1d6;
      }
      const auto next = rotateLeft(a, 5) + mixed + e + constant + words[round];
      e = d;
      d = c;
      c = rotateLeft(b, 30);
      b = a;
      a = next;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
  }
  std::array<std::uint8_t, 20> digest{};
  for (int index = 0; index < 20; ++index) {
    digest[index] = static_cast<std::uint8_t>(state[index / 4] >> (24 - (index % 4) * 8));
  }
  return digest;
}

} // namespace rntizen
