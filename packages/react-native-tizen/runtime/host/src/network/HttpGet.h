#pragma once

#include <string>

namespace rntizen {

/// Fetches `url` and returns the body; throws std::runtime_error naming the URL
/// when the server is unreachable or answers anything but 200.
std::string httpGet(const std::string &url);

} // namespace rntizen
