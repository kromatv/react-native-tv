#pragma once

#include <string>
#include <string_view>

namespace rntizen::expo::speech {

/// An `error` event of expo-speech-recognition: a Web Speech error code and why.
struct SpeechError {
  std::string code;
  std::string message;
};

/// The Web Speech error an STT error stands for. `message` is the engine's own,
/// which tells apart the failures it reports as one operation failure.
SpeechError speechErrorFor(int sttError, std::string_view message);

} // namespace rntizen::expo::speech
