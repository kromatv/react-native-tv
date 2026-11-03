#include "Uuid.h"

#include "network/Digest.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <random>

namespace rntizen::expo {

namespace {

using UuidBytes = std::array<unsigned char, 16>;

std::string format(const UuidBytes &bytes) {
  char text[37];
  std::snprintf(
      text,
      sizeof text,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
      bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
  return text;
}

std::optional<UuidBytes> parse(const std::string &text) {
  if (text.size() != 36) return std::nullopt;
  UuidBytes bytes{};
  std::size_t byte = 0;
  for (std::size_t index = 0; index < text.size();) {
    if (index == 8 || index == 13 || index == 18 || index == 23) {
      if (text[index] != '-') return std::nullopt;
      ++index;
      continue;
    }
    if (!std::isxdigit(static_cast<unsigned char>(text[index])) ||
        !std::isxdigit(static_cast<unsigned char>(text[index + 1]))) {
      return std::nullopt;
    }
    bytes[byte++] = static_cast<unsigned char>(std::stoi(text.substr(index, 2), nullptr, 16));
    index += 2;
  }
  return bytes;
}

void stamp(UuidBytes &bytes, int version) {
  bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | (version << 4));
  bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80);
}

} // namespace

std::string uuidV4() {
  std::random_device entropy;
  UuidBytes bytes{};
  for (auto &byte : bytes) byte = static_cast<unsigned char>(entropy() & 0xff);
  stamp(bytes, 4);
  return format(bytes);
}

std::optional<std::string> uuidV5(const std::string &name, const std::string &namespaceUuid) {
  auto space = parse(namespaceUuid);
  if (!space) return std::nullopt;
  std::string input(space->begin(), space->end());
  input += name;
  const auto digest = sha1(input);
  UuidBytes bytes{};
  std::copy_n(digest.begin(), bytes.size(), bytes.begin());
  stamp(bytes, 5);
  return format(bytes);
}

} // namespace rntizen::expo
