#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// The bytes of a `file:` URI's file or of a `data:` URI (RFC 8089, RFC 2397); nullopt for
/// any other scheme or a file that cannot be read.
std::optional<std::string> uriBytes(std::string_view uri);

} // namespace rntizen
