#include "Uuid.h"

#include "network/Digest.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <format>
#include <iterator>
#include <random>
#include <span>

namespace rntizen::expo {

namespace {

using UuidBytes = std::array<std::byte, 16>;

constexpr bool isDashAt(std::size_t index) {
  return index == 8 || index == 13 || index == 18 || index == 23;
}

std::string uuidText(const UuidBytes &bytes) {
  std::string text;
  text.reserve(36);
  for (std::size_t index = 0; index < bytes.size(); ++index) {
    if (index == 4 || index == 6 || index == 8 || index == 10) text += '-';
    std::format_to(std::back_inserter(text), "{:02x}", std::to_integer<unsigned>(bytes[index]));
  }
  return text;
}

std::optional<std::byte> hexByte(std::string_view digits) {
  unsigned value = 0;
  const auto *end = digits.data() + digits.size();
  const auto [last, error] = std::from_chars(digits.data(), end, value, 16);
  if (error != std::errc() || last != end) return std::nullopt;
  return static_cast<std::byte>(value);
}

std::optional<UuidBytes> parse(std::string_view text) {
  if (text.size() != 36) return std::nullopt;
  UuidBytes bytes{};
  std::size_t byte = 0;
  std::size_t index = 0;
  while (index < text.size()) {
    if (isDashAt(index)) {
      if (text[index] != '-') return std::nullopt;
      ++index;
      continue;
    }
    const auto value = hexByte(text.substr(index, 2));
    if (!value.has_value()) return std::nullopt;
    bytes[byte++] = *value;
    index += 2;
  }
  return bytes;
}

void stamp(UuidBytes &bytes, int version) {
  bytes[6] = (bytes[6] & std::byte{0x0f}) | static_cast<std::byte>(version << 4);
  bytes[8] = (bytes[8] & std::byte{0x3f}) | std::byte{0x80};
}

} // namespace

std::string uuidV4() {
  std::random_device entropy;
  UuidBytes bytes{};
  for (auto &byte : bytes) byte = static_cast<std::byte>(entropy() & 0xff);
  stamp(bytes, 4);
  return uuidText(bytes);
}

std::optional<std::string> uuidV5(std::string_view name, std::string_view namespaceUuid) {
  const auto space = parse(namespaceUuid);
  if (!space.has_value()) return std::nullopt;
  std::string input;
  input.reserve(space->size() + name.size());
  std::ranges::transform(*space, std::back_inserter(input), [](std::byte byte) { return std::to_integer<char>(byte); });
  input += name;
  const auto digest = sha1(input);
  UuidBytes bytes{};
  std::ranges::transform(
      std::span(digest).first<std::tuple_size_v<UuidBytes>>(), bytes.begin(), [](std::uint8_t byte) {
        return static_cast<std::byte>(byte);
      });
  stamp(bytes, 5);
  return uuidText(bytes);
}

} // namespace rntizen::expo
