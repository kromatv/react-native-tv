#pragma once

#include <string>

namespace rntizen {

/// Fetches `url` over TLS 1.2 or later, or plain HTTP, and returns the body;
/// throws NetworkError naming the URL
/// when the server is unreachable or answers anything but 200.
std::string httpGet(const std::string &url);

} // namespace rntizen
