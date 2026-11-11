#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>

namespace rnwebos {

/// Where log lines are streamed during development.
struct LogEndpoint {
  std::string host;
  std::uint16_t port;
};

/// `react-native-webos.json`, written into the package's `res` directory by
/// `rn-webos`. A bundle comes from `bundleUrl` (a dev server) when set, from the
/// packaged `bundleFile` otherwise. See docs/runtime-config.md.
struct HostConfig {
  std::string appKey;
  /// The webOS package id, which the log stream and the control link name so
  /// `rn-webos` tells apart the builds on one TV. Empty in a config without it.
  std::string packageId;
  std::string bundleFile;
  std::optional<std::string> bundleUrl;
  std::optional<LogEndpoint> logEndpoint;
  bool development;
  std::string resourceDir;
  /// expo-video's `supportsBackgroundPlayback`: players may keep playing while the app is hidden.
  bool backgroundMedia;
};

/// `react-native-webos.json` is missing, too large, or has a bad field, which the message names.
class HostConfigError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Reads and validates the config in `resourceDir`; throws HostConfigError naming the bad field.
HostConfig loadHostConfig(const std::string &resourceDir);

} // namespace rnwebos
