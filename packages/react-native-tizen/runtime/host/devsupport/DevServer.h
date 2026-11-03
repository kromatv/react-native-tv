#pragma once

#include "modules/DeviceProfile.h"

#include <folly/dynamic.h>

#include <cstdint>
#include <optional>
#include <string>

namespace rntizen {

/// The Metro or Expo dev server a bundle URL points at, and the URLs React
/// Native's tooling reaches it on, as Android's DevServerHelper builds them
/// (docs/reference/react-native-dev-tooling.md, section 1).
struct DevServer {
  std::string scheme;
  /// As the URL writes it: an IPv6 address keeps its brackets.
  std::string host;
  std::uint16_t port;
  /// The bundle's path without its leading slash: HMRClient.setup's `bundleEntry`.
  std::string bundlePath;
  /// The bundle URL's `app` parameter, the package id rn-tizen wrote there.
  std::string app;

  /// `host:port`.
  std::string authority() const;
  /// `path` on the server, over HTTP: `http://host:port/status`.
  std::string httpUrl(const std::string &path) const;
  /// `path` on the server, over a WebSocket: `ws://host:port/message`.
  std::string socketUrl(const std::string &path) const;
  /// The packager socket, `/message`, named as Android's JSPackagerClient names it.
  std::string packagerUrl(const std::string &deviceName) const;
  /// The arguments of `HMRClient.setup`, as Android's DevSupportManager passes them.
  folly::dynamic hmrClientSetup(bool hotLoadingEnabled) const;
};

/// What the dev server lists the TV as: "<model> - Tizen <version>", as
/// Android's getFriendlyDeviceName gives "<model> - <release> - API <level>".
std::string friendlyDeviceName(const DeviceProfile &device);

/// The dev server of an `http` or `https` bundle URL; nullopt for any other.
std::optional<DevServer> devServerOf(const std::string &bundleUrl);

/// `value` percent-encoded for a URL's query.
std::string encodeQueryValue(const std::string &value);

} // namespace rntizen
