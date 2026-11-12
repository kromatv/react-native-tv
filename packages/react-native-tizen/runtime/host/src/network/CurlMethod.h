#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

/// How libcurl sends a request's method and body. As Android's networking
/// does, a GET or HEAD goes without the body JavaScript may have given, and a
/// POST, PUT or PATCH without one carries an empty body.
struct CurlMethod {
  /// `CURLOPT_NOBODY`: a HEAD request.
  bool noBody{false};
  /// `CURLOPT_CUSTOMREQUEST`, when the method is not GET or HEAD.
  std::optional<std::string> customRequest;
  /// `CURLOPT_POSTFIELDS`, when the request carries a body.
  std::optional<std::string_view> body;
};

/// `body` must outlive the result, which points into it.
CurlMethod curlMethodFor(const std::string &method, const std::optional<std::string> &body);

} // namespace rntizen
