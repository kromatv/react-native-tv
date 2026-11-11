#pragma once

#include "HeaderPairs.h"

#include <string>
#include <string_view>
#include <vector>

namespace rntizen {

/// One part of a `multipart/form-data` body: its headers, then its bytes.
struct MultipartPart {
  HeaderPairs headers;
  std::string body;
};

/// A boundary of 32 random hex digits after a fixed prefix (RFC 2046 section 5.1.1 allows 70 characters).
std::string multipartBoundary();

/// The body RFC 7578 describes: each part after `--boundary`, then `--boundary--`.
std::string encodeMultipart(const std::vector<MultipartPart> &parts, std::string_view boundary);

} // namespace rntizen
