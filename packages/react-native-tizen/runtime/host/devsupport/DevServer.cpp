#include "DevServer.h"

#include <cctype>
#include <charconv>
#include <cstddef>
#include <format>
#include <string_view>

namespace rntizen {

namespace {

constexpr const char *kPlatform = "tizen";
constexpr const char *kClientId = "DevSupport";
constexpr std::uint16_t kHttpPort = 80;
constexpr std::uint16_t kHttpsPort = 443;

std::optional<std::uint16_t> parsePort(std::string_view text) {
  std::uint16_t port = 0;
  const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), port);
  if (error != std::errc() || end != text.data() + text.size() || port == 0) return std::nullopt;
  return port;
}

std::string queryValue(std::string_view query, std::string_view name) {
  while (!query.empty()) {
    const auto separator = query.find('&');
    if (const auto pair = query.substr(0, separator);
        pair.size() > name.size() && pair.starts_with(name) && pair[name.size()] == '=') {
      return std::string(pair.substr(name.size() + 1));
    }
    if (separator == std::string_view::npos) break;
    query.remove_prefix(separator + 1);
  }
  return {};
}

std::string_view withoutFragment(std::string_view location) {
  return location.substr(0, location.find('#'));
}

struct Authority {
  std::string_view host;
  std::optional<std::uint16_t> port;
};

Authority splitAuthority(std::string_view authority, std::uint16_t defaultPort) {
  const bool bracketed = authority.starts_with('[');
  auto hostEnd = bracketed ? authority.find(']') : authority.rfind(':');
  if (bracketed && hostEnd != std::string_view::npos) hostEnd += 1;
  if (hostEnd == std::string_view::npos || hostEnd == authority.size()) return {authority, defaultPort};
  if (authority[hostEnd] != ':') return {authority, std::nullopt};
  return {authority.substr(0, hostEnd), parsePort(authority.substr(hostEnd + 1))};
}

bool isUnreserved(char character) {
  return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '-' || character == '_' ||
      character == '.' || character == '~';
}

} // namespace

std::string DevServer::authority() const {
  return std::format("{}:{}", host, port);
}

std::string DevServer::httpUrl(const std::string &path) const {
  return std::format("{}://{}{}", scheme, authority(), path);
}

std::string DevServer::socketUrl(const std::string &path) const {
  return std::format("{}://{}{}", scheme == "https" ? "wss" : "ws", authority(), path);
}

std::string DevServer::packagerUrl(const std::string &deviceName) const {
  return socketUrl(std::format(
      "/message?device={}&app={}&clientid={}", encodeQueryValue(deviceName), encodeQueryValue(app), kClientId));
}

folly::dynamic DevServer::hmrClientSetup(bool hotLoadingEnabled) const {
  return folly::dynamic::array(kPlatform, bundlePath, host, port, hotLoadingEnabled, scheme);
}

std::string friendlyDeviceName(const DeviceProfile &device) {
  return std::format("{} - Tizen {}", device.modelName, device.platformVersion);
}

std::optional<DevServer> devServerOf(const std::string &bundleUrl) {
  const std::string_view url(bundleUrl);
  const auto schemeEnd = url.find("://");
  if (schemeEnd == std::string_view::npos) return std::nullopt;
  const auto scheme = url.substr(0, schemeEnd);
  if (scheme != "http" && scheme != "https") return std::nullopt;
  const auto rest = url.substr(schemeEnd + 3);
  const auto authorityEnd = rest.find_first_of("/?#");
  const auto [host, port] = splitAuthority(rest.substr(0, authorityEnd), scheme == "https" ? kHttpsPort : kHttpPort);
  if (host.empty() || !port.has_value()) return std::nullopt;

  const auto location =
      authorityEnd == std::string_view::npos ? std::string_view() : withoutFragment(rest.substr(authorityEnd));
  const auto queryStart = location.find('?');
  auto path = location.substr(0, queryStart);
  if (path.starts_with('/')) path.remove_prefix(1);
  const auto query = queryStart == std::string_view::npos ? std::string_view() : location.substr(queryStart + 1);
  return DevServer{
      .scheme = std::string(scheme),
      .host = std::string(host),
      .port = *port,
      .bundlePath = std::string(path),
      .app = queryValue(query, "app"),
  };
}

std::string encodeQueryValue(const std::string &value) {
  static constexpr std::string_view kHex = "0123456789ABCDEF";
  std::string encoded;
  for (const char character : value) {
    const auto byte = static_cast<std::byte>(character);
    if (isUnreserved(character)) {
      encoded += character;
    } else {
      encoded += '%';
      encoded += kHex[std::to_integer<std::size_t>(byte >> 4)];
      encoded += kHex[std::to_integer<std::size_t>(byte & std::byte{0x0f})];
    }
  }
  return encoded;
}

} // namespace rntizen
