#pragma once

#include <jsi/jsi.h>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace rntizen {

/// What a request sends, read from the `data` object React Native's XMLHttpRequest hands to
/// `sendRequest` (`Libraries/Network/convertRequestBody.js`).
struct RequestBody {
  std::optional<std::string> bytes;
  /// Replaces the request's Content-Type: a multipart body names its boundary there.
  std::optional<std::string> contentType;
};

/// A body that cannot be sent; its message is the request's error, as on Android.
class RequestBodyError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// `contentType` is the request's Content-Type header, empty when it has none. Throws RequestBodyError.
RequestBody readRequestBody(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::Object &data,
    std::string_view contentType);

} // namespace rntizen
