#include "RequestBody.h"

#include "HeaderPairs.h"
#include "MultipartBody.h"
#include "UriBytes.h"

#include "../blob/BlobReference.h"
#include "../../network/Base64.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <vector>

namespace rntizen {

namespace {

using facebook::jsi::Array;
using facebook::jsi::Object;
using facebook::jsi::Runtime;

std::string uriBody(std::string_view uri) {
  auto bytes = uriBytes(uri);
  if (!bytes.has_value()) throw RequestBodyError(std::format("Could not retrieve file for uri {}", uri));
  return std::move(*bytes);
}

bool fitsInHeader(std::string_view text) {
  return text.find_first_of(std::string_view("\r\n\0", 3)) == std::string_view::npos;
}

bool isMultipart(std::string_view type) {
  constexpr std::string_view prefix = "multipart/";
  return type.size() > prefix.size() &&
         std::ranges::equal(type.substr(0, prefix.size()), prefix, [](unsigned char a, unsigned char b) {
           return std::tolower(a) == b;
         });
}

HeaderPairs partHeaders(Runtime &runtime, const Object &part) {
  const auto headers = part.getProperty(runtime, "headers");
  if (!headers.isObject() || !headers.getObject(runtime).isArray(runtime)) {
    throw RequestBodyError("Missing or invalid header format for FormData part.");
  }
  auto pairs = readHeaderPairs(runtime, headers.getObject(runtime).getArray(runtime));
  if (!std::ranges::all_of(
          pairs, [](const auto &header) { return fitsInHeader(header.first) && fitsInHeader(header.second); })) {
    throw RequestBodyError("Invalid header in FormData part.");
  }
  return pairs;
}

MultipartPart formDataPart(Runtime &runtime, const Object &part) {
  auto headers = partHeaders(runtime, part);
  const std::string type(headerValue(headers, "content-type"));
  if (!type.empty()) setHeader(headers, "Content-Type", type);
  if (const auto text = part.getProperty(runtime, "string"); text.isString()) {
    return {.headers = std::move(headers), .body = text.getString(runtime).utf8(runtime)};
  }
  if (const auto uri = part.getProperty(runtime, "uri"); uri.isString()) {
    if (type.empty()) throw RequestBodyError("Binary FormData part needs a content-type header.");
    return {.headers = std::move(headers), .body = uriBody(uri.getString(runtime).utf8(runtime))};
  }
  throw RequestBodyError("Unrecognized FormData part.");
}

RequestBody formDataBody(Runtime &runtime, const Array &parts, std::string_view contentType) {
  const std::string type = contentType.empty() ? std::string("multipart/form-data") : std::string(contentType);
  if (!isMultipart(type)) throw RequestBodyError(std::format("Invalid media type for FormData: {}", type));
  std::vector<MultipartPart> encoded;
  for (std::size_t index = 0, size = parts.size(runtime); index < size; ++index) {
    const auto part = parts.getValueAtIndex(runtime, index);
    if (!part.isObject()) throw RequestBodyError("Unrecognized FormData part.");
    encoded.push_back(formDataPart(runtime, part.getObject(runtime)));
  }
  const auto boundary = multipartBoundary();
  return {.bytes = encodeMultipart(encoded, boundary), .contentType = std::format("{}; boundary={}", type, boundary)};
}

} // namespace

RequestBody readRequestBody(Runtime &runtime, const Object &data, std::string_view contentType) {
  if (const auto text = data.getProperty(runtime, "string"); text.isString()) {
    return {.bytes = text.getString(runtime).utf8(runtime), .contentType = std::nullopt};
  }
  if (const auto encoded = data.getProperty(runtime, "base64"); encoded.isString()) {
    return {.bytes = decodeBase64(encoded.getString(runtime).utf8(runtime)), .contentType = std::nullopt};
  }
  if (const auto blob = data.getProperty(runtime, "blob"); blob.isObject()) {
    return {.bytes = resolveBlob(readBlobReference(runtime, blob.getObject(runtime))), .contentType = std::nullopt};
  }
  if (const auto uri = data.getProperty(runtime, "uri"); uri.isString()) {
    if (contentType.empty()) throw RequestBodyError("Payload is set but no content-type header specified");
    return {.bytes = uriBody(uri.getString(runtime).utf8(runtime)), .contentType = std::nullopt};
  }
  if (const auto parts = data.getProperty(runtime, "formData"); parts.isObject() && parts.getObject(runtime).isArray(runtime)) {
    return formDataBody(runtime, parts.getObject(runtime).getArray(runtime), contentType);
  }
  return {.bytes = std::nullopt, .contentType = std::nullopt};
}

} // namespace rntizen
