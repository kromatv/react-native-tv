#include "SearchUrl.h"

#include <cstdio>

namespace rntizen {

namespace {

bool isUnreserved(unsigned char character) {
  return (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z') ||
      (character >= '0' && character <= '9') || character == '-' || character == '.' || character == '_' ||
      character == '~';
}

std::string percentEncode(const std::string &text) {
  std::string encoded;
  for (const unsigned char character : text) {
    if (isUnreserved(character)) {
      encoded += static_cast<char>(character);
      continue;
    }
    char escape[4];
    std::snprintf(escape, sizeof escape, "%%%02X", character);
    encoded += escape;
  }
  return encoded;
}

std::string field(const std::map<std::string, std::string> &fields, const std::string &name) {
  const auto found = fields.find(name);
  return found == fields.end() ? std::string() : found->second;
}

std::string placeholderValue(const std::map<std::string, std::string> &fields, const std::string &name) {
  if (name != "query") return field(fields, name);
  const auto title = field(fields, "title");
  return title.empty() ? field(fields, "utterance") : title;
}

} // namespace

std::string fillSearchUrl(const std::string &templateUrl, const std::map<std::string, std::string> &fields) {
  std::string url;
  std::size_t position = 0;
  while (position < templateUrl.size()) {
    const auto open = templateUrl.find('{', position);
    const auto close = open == std::string::npos ? std::string::npos : templateUrl.find('}', open);
    if (close == std::string::npos) {
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
