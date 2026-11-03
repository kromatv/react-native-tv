#pragma once

#include <string>
#include <string_view>

namespace rntizen {

/// Decodes as the WHATWG Encoding Standard's UTF-8 decoder does: one U+FFFD
/// for each maximal invalid subpart (docs/reference/text-input.md).
std::u32string decodeUtf8(std::string_view utf8);
std::string encodeUtf8(std::u32string_view text);

} // namespace rntizen
