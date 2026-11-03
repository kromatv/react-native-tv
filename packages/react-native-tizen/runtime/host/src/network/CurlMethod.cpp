#include "CurlMethod.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace rntizen {

namespace {

constexpr std::array kMethodsWithBody{"POST", "PUT", "PATCH"};

std::string uppercase(std::string_view text) {
  std::string result(text);
  std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
  return result;
}

} // namespace

CurlMethod curlMethodFor(const std::string &method, const std::optional<std::string> &body) {
  const auto normalized = uppercase(method);
  if (normalized == "GET") return {};
  if (normalized == "HEAD") return {.noBody = true, .customRequest = std::nullopt, .body = std::nullopt};
  CurlMethod sent{.noBody = false, .customRequest = method, .body = std::nullopt};
  if (body.has_value()) {
    sent.body = *body;
  } else if (std::ranges::find(kMethodsWithBody, normalized) != kMethodsWithBody.end()) {
    sent.body = std::string_view();
  }
  return sent;
}

} // namespace rntizen
