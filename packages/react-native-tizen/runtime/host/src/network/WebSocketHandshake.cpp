#include "WebSocketHandshake.h"

#include "Base64.h"
#include "Digest.h"
#include "HeaderField.h"
#include "NetworkError.h"

#include <curl/curl.h>

#include <algorithm>
#include <cctype>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>

namespace rntizen {

namespace {

constexpr std::string_view kAcceptGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
constexpr const char *kUserAgent = "react-native-tizen";

std::string lowercase(std::string_view text) {
  std::string result(text);
  std::ranges::transform(result, result.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return result;
}

std::string trimmed(std::string_view text) {
  const auto first = text.find_first_not_of(" \t");
  if (first == std::string_view::npos) return {};
  const auto last = text.find_last_not_of(" \t");
  return std::string(text.substr(first, last - first + 1));
}

std::optional<std::string> urlPart(const CURLU *url, CURLUPart part) {
  char *value = nullptr;
  if (curl_url_get(url, part, &value, 0) != CURLUE_OK) return std::nullopt;
  std::string result(value);
  curl_free(value);
  return result;
}

bool hasToken(std::string_view list, std::string_view token) {
  std::size_t start = 0;
  while (start <= list.size()) {
    const auto comma = std::min(list.find(',', start), list.size());
    if (lowercase(trimmed(list.substr(start, comma - start))) == token) return true;
    start = comma + 1;
  }
  return false;
}

using Headers = std::map<std::string, std::string, std::less<>>;

Headers responseHeaders(std::string_view head, std::string &statusLine) {
  Headers headers;
  std::size_t start = 0;
  bool first = true;
  while (start < head.size()) {
    const auto end = std::min(head.find("\r\n", start), head.size());
    const auto line = head.substr(start, end - start);
    start = end + 2;
    if (first) {
      statusLine = std::string(line);
      first = false;
      continue;
    }
    const auto colon = line.find(':');
    if (colon == std::string_view::npos) continue;
    auto &value = headers[lowercase(trimmed(line.substr(0, colon)))];
    value += (value.empty() ? "" : ", ") + trimmed(line.substr(colon + 1));
  }
  return headers;
}

bool namedIn(const std::vector<std::pair<std::string, std::string>> &headers, std::string_view name) {
  return std::ranges::any_of(headers, [name](const auto &header) { return lowercase(header.first) == name; });
}

} // namespace

WebSocketEndpoint parseWebSocketUrl(const std::string &url) {
  std::unique_ptr<CURLU, decltype(&curl_url_cleanup)> parsed(curl_url(), &curl_url_cleanup);
  if (curl_url_set(parsed.get(), CURLUPART_URL, url.c_str(), CURLU_NON_SUPPORT_SCHEME) != CURLUE_OK) {
    throw std::invalid_argument(std::format("Invalid WebSocket URL {}", url));
  }
  const auto scheme = lowercase(urlPart(parsed.get(), CURLUPART_SCHEME).value_or(""));
  const bool secure = scheme == "wss" || scheme == "https";
  if (!secure && scheme != "ws" && scheme != "http") {
    throw std::invalid_argument(std::format("Unsupported WebSocket URL {}", url));
  }
  const auto host = urlPart(parsed.get(), CURLUPART_HOST).value_or("");
  const auto port = urlPart(parsed.get(), CURLUPART_PORT);
  const auto authority = port ? host + ":" + *port : host;
  const auto query = urlPart(parsed.get(), CURLUPART_QUERY);
  return WebSocketEndpoint{
      .connectUrl = std::string(secure ? "https://" : "http://") + authority + "/",
      .authority = authority,
      .target = urlPart(parsed.get(), CURLUPART_PATH).value_or("/") + (query ? "?" + *query : ""),
      .origin = std::string(secure ? "https://" : "http://") + authority,
  };
}

std::string makeWebSocketKey() {
  std::random_device entropy;
  std::string bytes(16, '\0');
  for (auto &byte : bytes) byte = static_cast<char>(entropy() & 0xff);
  return encodeBase64(bytes);
}

std::string webSocketAccept(std::string_view key) {
  const auto digest = sha1(std::string(key) + std::string(kAcceptGuid));
  return encodeBase64(std::string_view(reinterpret_cast<const char *>(digest.data()), digest.size()));
}

std::string buildWebSocketHandshake(
    const WebSocketEndpoint &endpoint,
    std::string_view key,
    const std::vector<std::string> &protocols,
    const std::vector<std::pair<std::string, std::string>> &headers) {
  if (const auto header = invalidHeader(headers)) {
    throw std::invalid_argument(std::format("The header {} holds a character HTTP forbids", *header));
  }
  std::string request = "GET " + endpoint.target + " HTTP/1.1\r\n";
  request += "Host: " + endpoint.authority + "\r\n";
  request += "Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\n";
  request += "Sec-WebSocket-Key: " + std::string(key) + "\r\n";
  if (!protocols.empty()) {
    std::string list;
    for (const auto &protocol : protocols) list += (list.empty() ? "" : ",") + protocol;
    request += "Sec-WebSocket-Protocol: " + list + "\r\n";
  }
  if (!namedIn(headers, "origin")) request += "Origin: " + endpoint.origin + "\r\n";
  if (!namedIn(headers, "user-agent")) request += std::string("User-Agent: ") + kUserAgent + "\r\n";
  for (const auto &[name, value] : headers) request += name + ": " + value + "\r\n";
  return request + "\r\n";
}

std::string checkWebSocketHandshake(
    std::string_view head,
    std::string_view key,
    const std::vector<std::string> &protocols) {
  std::string statusLine;
  const auto headers = responseHeaders(head, statusLine);
  const auto space = statusLine.find(' ');
  if (const auto status = space == std::string::npos ? std::string() : statusLine.substr(space + 1, 3); status != "101") {
    const auto answer = space == std::string::npos ? statusLine : statusLine.substr(space + 1);
    throw NetworkError(std::format("Expected HTTP 101 response but was '{}'", answer));
  }
  const auto header = [&headers](const char *name) {
    auto found = headers.find(name);
    return found == headers.end() ? std::string() : found->second;
  };
  const auto require = [](bool holds, const char *problem) {
    if (!holds) throw NetworkError(problem);
  };
  require(hasToken(header("upgrade"), "websocket"), "The server did not upgrade to websocket");
  require(hasToken(header("connection"), "upgrade"), "The server did not answer Connection: Upgrade");
  require(header("sec-websocket-accept") == webSocketAccept(key), "The server answered a wrong Sec-WebSocket-Accept");
  require(header("sec-websocket-extensions").empty(), "The server enabled an extension nobody asked for");
  auto protocol = header("sec-websocket-protocol");
  if (!protocol.empty() && std::ranges::find(protocols, protocol) == protocols.end()) {
    throw NetworkError(std::format("The server picked a protocol nobody asked for: {}", protocol));
  }
  return protocol;
}

} // namespace rntizen
