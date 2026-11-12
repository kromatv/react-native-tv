#include "HeaderField.h"

#include <algorithm>

namespace rntizen {

namespace {

bool isTokenSymbol(char character) {
  switch (character) {
    case '!':
    case '#':
    case '$':
    case '%':
    case '&':
    case '\'':
    case '*':
    case '+':
    case '-':
    case '.':
    case '^':
    case '_':
    case '`':
    case '|':
    case '~':
      return true;
    default:
      return false;
  }
}

bool isTokenCharacter(char character) {
  const auto byte = static_cast<unsigned char>(character);
  const bool alphanumeric = (byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') || (byte >= 'a' && byte <= 'z');
  return alphanumeric || isTokenSymbol(character);
}

} // namespace

bool isHeaderName(std::string_view name) {
  return !name.empty() && std::ranges::all_of(name, isTokenCharacter);
}

bool isHeaderValue(std::string_view value) {
  return value.find_first_of(std::string_view("\0\r\n", 3)) == std::string_view::npos;
}

std::optional<std::string> invalidHeader(const std::vector<std::pair<std::string, std::string>> &headers) {
  for (const auto &[name, value] : headers) {
    if (!isHeaderName(name) || !isHeaderValue(value)) return name;
  }
  return std::nullopt;
}

} // namespace rntizen
