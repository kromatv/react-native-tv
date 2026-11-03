#include "HeaderPairs.h"

#include <algorithm>
#include <cctype>

namespace rntizen {

namespace {

bool sameName(std::string_view left, std::string_view right) {
  return std::ranges::equal(left, right, [](unsigned char a, unsigned char b) {
    return std::tolower(a) == std::tolower(b);
  });
}

} // namespace

HeaderPairs readHeaderPairs(facebook::jsi::Runtime &runtime, const facebook::jsi::Array &headers) {
  HeaderPairs pairs;
  for (std::size_t index = 0; index < headers.size(runtime); ++index) {
    auto pair = headers.getValueAtIndex(runtime, index).asObject(runtime).asArray(runtime);
    pairs.emplace_back(
        pair.getValueAtIndex(runtime, 0).toString(runtime).utf8(runtime),
        pair.getValueAtIndex(runtime, 1).toString(runtime).utf8(runtime));
  }
  return pairs;
}

std::string_view headerValue(const HeaderPairs &headers, std::string_view name) {
  const auto found = std::ranges::find_if(headers, [name](const auto &header) { return sameName(header.first, name); });
  return found == headers.end() ? std::string_view() : std::string_view(found->second);
}

void setHeader(HeaderPairs &headers, std::string_view name, std::string_view value) {
  std::erase_if(headers, [name](const auto &header) { return sameName(header.first, name); });
  headers.emplace_back(name, value);
}

} // namespace rntizen
