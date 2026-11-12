#pragma once

#include <folly/dynamic.h>

#include <string>

namespace rntizen::expo::speech {

/// What `start()` asks for, read from expo-speech-recognition's options.
struct SpeechOptions {
  /// A BCP 47 tag; empty for the TV's own language.
  std::string language;
  bool interimResults{false};
  int maxAlternatives{1};
};

/// expo-speech-recognition's `getStateAsync()` states.
enum class SpeechState { Inactive, Starting, Recognizing, Stopping };

/// Reads `start()`'s options; a field that is missing or of another type keeps its default.
SpeechOptions speechOptionsFrom(const folly::dynamic &options);

/// The name `getStateAsync()` answers for `state`.
const char *speechStateName(SpeechState state);

} // namespace rntizen::expo::speech
