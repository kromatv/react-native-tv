#include "Base64.h"

#include <array>
#include <cstdint>
#include <stdexcept>

namespace rntizen {

namespace {

constexpr std::string_view kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

consteval std::array<int, 256> decodingTable() {
  std::array<int, 256> table{};
  table.fill(-1);
  for (std::size_t index = 0; index < kAlphabet.size(); ++index) {
    table[static_cast<unsigned char>(kAlphabet[index])] = static_cast<int>(index);
  }
  return table;
}

constexpr auto kDecoding = decodingTable();

bool isWhitespace(char character) {
  return character == ' ' || character == '\n' || character == '\r' || character == '\t';
}

} // namespace

std::string encodeBase64(std::string_view bytes) {
  std::string encoded;
  encoded.reserve((bytes.size() + 2) / 3 * 4);
  std::size_t index = 0;
  for (; index + 2 < bytes.size(); index += 3) {
    const auto chunk = (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[index])) << 16) |
        (static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[index + 1])) << 8) |
        static_cast<unsigned char>(bytes[index + 2]);
    encoded += kAlphabet[(chunk >> 18) & 63];
    encoded += kAlphabet[(chunk >> 12) & 63];
    encoded += kAlphabet[(chunk >> 6) & 63];
    encoded += kAlphabet[chunk & 63];
  }
  const auto remaining = bytes.size() - index;
  if (remaining == 0) return encoded;
  std::uint32_t chunk = static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[index])) << 16;
  if (remaining == 2) chunk |= static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[index + 1])) << 8;
  encoded += kAlphabet[(chunk >> 18) & 63];
  encoded += kAlphabet[(chunk >> 12) & 63];
  encoded += remaining == 2 ? kAlphabet[(chunk >> 6) & 63] : '=';
  encoded += '=';
  return encoded;
}

std::string decodeBase64(std::string_view text) {
  std::string decoded;
  decoded.reserve(text.size() / 4 * 3);
  std::uint32_t buffer = 0;
  int bits = 0;
  for (char character : text) {
    if (character == '=') break;
    if (isWhitespace(character)) continue;
    const auto value = kDecoding[static_cast<unsigned char>(character)];
    if (value < 0) throw std::invalid_argument("invalid base64");
    buffer = (buffer << 6) | static_cast<std::uint32_t>(value);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      decoded += static_cast<char>((buffer >> bits) & 0xff);
    }
  }
  return decoded;
}

} // namespace rntizen
