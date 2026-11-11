#pragma once

#include <string>
#include <string_view>

namespace rntizen {

std::string encodeBase64(std::string_view bytes);

/// Decodes standard base64, skipping whitespace; throws std::invalid_argument on any other non-alphabet character.
std::string decodeBase64(std::string_view text);

} // namespace rntizen
