#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace rntizen {

enum class PackagerCommand { Reload, DevMenu };

/// What one text from the dev server's `/message` asks of the app: a command,
/// or, for a request the app has no handler for, the error to answer with.
struct PackagerMessage {
  std::optional<PackagerCommand> command;
  std::optional<std::string> reply;
};

/// Reads a message of the packager protocol, version 2; any other text asks nothing.
PackagerMessage readPackagerMessage(std::string_view text);

} // namespace rntizen
