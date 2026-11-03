#include "SearchUrl.h"

#include <cstddef>
#include <format>

namespace rntizen {

namespace {

using Fields = std::map<std::string, std::string, std::less<>>;

bool isUnreserved(char character) {
  return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
      (character >= '0' && character <= '9') || character == '-' || character == '.' || character == '_' ||
      character == '~';
}

std::string percentEncode(std::string_view text) {
  std::string encoded;
  for (const char character : text) {
    if (isUnreserved(character)) {
      encoded += character;
    } else {
      encoded += std::format("%{:02X}", std::to_integer<unsigned int>(static_cast<std::byte>(character)));
    }
  }
  return encoded;
}

std::string_view field(const Fields &fields, std::string_view name) {
  const auto found = fields.find(name);
  return found == fields.end() ? std::string_view() : std::string_view(found->second);
}

std::string_view placeholderValue(const Fields &fields, std::string_view name) {
  if (name != "query") return field(fields, name);
  const auto title = field(fields, "title");
  return title.empty() ? field(fields, "utterance") : title;
}

} // namespace

std::string fillSearchUrl(std::string_view templateUrl, const Fields &fields) {
  std::string url;
  std::size_t position = 0;
  while (position < templateUrl.size()) {
    const auto open = templateUrl.find('{', position);
    const auto close = open == std::string_view::npos ? std::string_view::npos : templateUrl.find('}', open);
    if (close == std::string_view::npos) {
      url += templateUrl.substr(position);
      break;
    }
    url += templateUrl.substr(position, open - position);
    url += percentEncode(placeholderValue(fields, templateUrl.substr(open + 1, close - open - 1)));
    position = close + 1;
  }
  return url;
}

} // namespace rntizen
