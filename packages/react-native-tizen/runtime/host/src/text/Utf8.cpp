#include "Utf8.h"

#include <cstddef>
#include <optional>

namespace rntizen {

namespace {

constexpr char32_t kReplacement = 0xFFFD;

struct Utf8Lead {
  std::size_t continuations;
  unsigned int lower;
  unsigned int upper;
  char32_t bits;
};

unsigned int octet(char byte) {
  return std::to_integer<unsigned int>(static_cast<std::byte>(byte));
}

std::optional<Utf8Lead> utf8Lead(unsigned int byte) {
  if (byte >= 0xC2 && byte <= 0xDF) return Utf8Lead{.continuations = 1, .lower = 0x80, .upper = 0xBF, .bits = byte & 0x1F};
  if (byte >= 0xE0 && byte <= 0xEF) {
    return Utf8Lead{
        .continuations = 2,
        .lower = byte == 0xE0 ? 0xA0U : 0x80U,
        .upper = byte == 0xED ? 0x9FU : 0xBFU,
        .bits = byte & 0x0F};
  }
  if (byte >= 0xF0 && byte <= 0xF4) {
    return Utf8Lead{
        .continuations = 3,
        .lower = byte == 0xF0 ? 0x90U : 0x80U,
        .upper = byte == 0xF4 ? 0x8FU : 0xBFU,
        .bits = byte & 0x07};
  }
  return std::nullopt;
}

char32_t continueSequence(std::string_view utf8, std::size_t &next, Utf8Lead lead) {
  char32_t codePoint = lead.bits;
  for (std::size_t seen = 0; seen < lead.continuations; ++seen) {
    if (next == utf8.size()) return kReplacement;
    const auto byte = octet(utf8[next]);
    if (byte < lead.lower || byte > lead.upper) return kReplacement;
    codePoint = (codePoint << 6) | (byte & 0x3F);
    lead.lower = 0x80;
    lead.upper = 0xBF;
    ++next;
  }
  return codePoint;
}

} // namespace

std::u32string decodeUtf8(std::string_view utf8) {
  std::u32string decoded;
  decoded.reserve(utf8.size());
  std::size_t next = 0;
  while (next < utf8.size()) {
    const auto byte = octet(utf8[next]);
    ++next;
    if (byte < 0x80) {
      decoded.push_back(byte);
    } else if (const auto lead = utf8Lead(byte)) {
      decoded.push_back(continueSequence(utf8, next, *lead));
    } else {
      decoded.push_back(kReplacement);
    }
  }
  return decoded;
}

std::string encodeUtf8(std::u32string_view text) {
  std::string encoded;
  encoded.reserve(text.size());
  for (const char32_t codePoint : text) {
    if (codePoint < 0x80) {
      encoded.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
      encoded.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
      encoded.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
      encoded.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
      encoded.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      encoded.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
      encoded.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
      encoded.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
      encoded.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
      encoded.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
  }
  return encoded;
}

} // namespace rntizen
