#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rntizen {

/// Whether `name` is a field name: one or more token characters (RFC 9110
/// section 5.1), what Fetch calls a header name.
bool isHeaderName(std::string_view name);

/// Whether `value` may go on the wire as a field value: Fetch's header value
/// holds no NUL, CR or LF, so it cannot end its line and start another.
bool isHeaderValue(std::string_view value);

/// The name of the first header in `headers` that is not a valid name and
/// value, or nullopt when every one is.
std::optional<std::string> invalidHeader(const std::vector<std::pair<std::string, std::string>> &headers);

} // namespace rntizen
