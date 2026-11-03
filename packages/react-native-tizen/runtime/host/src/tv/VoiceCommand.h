#pragma once

#include <folly/dynamic.h>

#include <map>
#include <optional>
#include <string>

namespace rntizen {

/// One utterance the shell received from Samsung's voice service, as the JSON
/// `{"type": ...}` it sends: a transport command, a search or a selection.
struct VoiceCommand {
  std::string type;
  double seconds{0};
  double position{0};
  bool enabled{false};
  int index{0};
  std::string title;
  std::map<std::string, std::string> search;

  bool isTransport() const;

  /// The payload JavaScript gets: a `VoiceCommand` or a `VoiceSearch`.
  folly::dynamic payload() const;
};

/// Parses and checks the shell's JSON; nullopt, after logging why, when it is not a command.
std::optional<VoiceCommand> parseVoiceCommand(const std::string &json);

} // namespace rntizen
