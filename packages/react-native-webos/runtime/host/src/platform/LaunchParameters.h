#pragma once

#include <folly/dynamic.h>

#include <optional>
#include <string>
#include <string_view>

namespace rnwebos {

/// What the TV's app manager launched the app with
/// (docs/reference/webos-tv-platform.md, section 3.2).
struct LaunchParameters {
  /// The `params` object the caller of `launch` gave; an empty object when it gave none.
  folly::dynamic parameters = folly::dynamic::object();
  /// Why the app manager started the app, such as `launcher`; empty when it does not say.
  std::string reason;
  /// The lifecycle interface the app manager names in its argument, which only version 2 does.
  std::optional<int> interfaceVersion;
};

/// Reads the launch argument, the one JSON string the app manager passes, in
/// any of the shapes its versions write. Anything else gives empty parameters:
/// no argument, as from a shell, or a path, as in the simulator.
LaunchParameters parseLaunchArgument(std::string_view argument);

/// The launch argument of a command line: its last argument, as the app manager passes it.
LaunchParameters launchParametersOf(int argc, char **argv);

/// The deep link in launch or relaunch parameters: `contentTarget`, the
/// parameter LG's Home and Search open content with. Nothing when there is none.
std::optional<std::string> deepLinkOf(const folly::dynamic &parameters);

} // namespace rnwebos
