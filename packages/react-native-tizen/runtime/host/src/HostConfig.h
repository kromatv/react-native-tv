#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace rntizen {

/// Where log lines are streamed during development.
struct LogEndpoint {
  std::string host;
  std::uint16_t port;
};

/// `react-native-tizen.json`, written into the package's `res` directory by
/// `rn-tizen`. A bundle comes from `bundleUrl` (a dev server) when set, from the
/// packaged `bundleFile` otherwise. See docs/runtime-config.md.
struct HostConfig {
  std::string appKey;
  std::string bundleFile;
  std::optional<std::string> bundleUrl;
  std::optional<LogEndpoint> logEndpoint;
  bool development;
  std::string resourceDir;
  /// `expo.tizen.resolution` is `4k`.
  bool uhd;
  /// expo-video's `supportsBackgroundPlayback`: the manifest declares the `media` background category.
  bool backgroundMedia;
  /// `expo.tizen.voice` is set.
  bool voice;
  std::optional<std::string> voiceSearchUrl;
};

/// Reads and validates the config in `resourceDir`; throws std::runtime_error naming the bad field.
HostConfig loadHostConfig(const std::string &resourceDir);

} // namespace rntizen
