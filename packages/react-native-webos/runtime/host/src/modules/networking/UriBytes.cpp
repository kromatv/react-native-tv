#include "UriBytes.h"

#include "../../network/Base64.h"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace rntizen {

namespace {

int hexValue(char digit) {
  if (digit >= '0' && digit <= '9') return digit - '0';
  if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
  if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
  return -1;
}

std::string percentDecoded(std::string_view text) {
  std::string decoded;
  decoded.reserve(text.size());
  std::size_t index = 0;
  while (index < text.size()) {
    const int high = index + 2 < text.size() && text[index] == '%' ? hexValue(text[index + 1]) : -1;
    const int low = high < 0 ? -1 : hexValue(text[index + 2]);
    if (low < 0) {
      decoded += text[index];
      ++index;
    } else {
      decoded += static_cast<char>(high * 16 + low);
      index += 3;
    }
  }
  return decoded;
}

std::optional<std::string> fileBytes(std::string_view rest) {
  if (rest.starts_with("localhost/")) rest.remove_prefix(std::string_view("localhost").size());
  if (!rest.starts_with('/')) return std::nullopt;
  std::ifstream file(percentDecoded(rest.substr(0, rest.find_first_of("?#"))), std::ios::binary);
  if (!file) return std::nullopt;
  std::string bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
  if (file.bad()) return std::nullopt;
  return bytes;
}

std::optional<std::string> dataBytes(std::string_view rest) {
  const auto comma = rest.find(',');
  if (comma == std::string_view::npos) return std::nullopt;
  const auto data = rest.substr(comma + 1);
  if (!rest.substr(0, comma).ends_with(";base64")) return percentDecoded(data);
  try {
    return decodeBase64(percentDecoded(data));
  } catch (const std::invalid_argument &) {
    return std::nullopt;
  }
}

} // namespace

std::optional<std::string> uriBytes(std::string_view uri) {
  constexpr std::string_view file = "file://";
  constexpr std::string_view data = "data:";
  if (uri.starts_with(file)) return fileBytes(uri.substr(file.size()));
  if (uri.starts_with(data)) return dataBytes(uri.substr(data.size()));
  return std::nullopt;
}

} // namespace rntizen
