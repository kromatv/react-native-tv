#include "SpeechErrors.h"

#include "SttLibrary.h"

namespace rntizen::expo::speech {

namespace {

std::string_view codeFor(int sttError, std::string_view message) {
  using namespace stt_error;
  switch (sttError) {
    case kNoSpeech:
      return "no-speech";
    case kRecordingTimedOut:
      return "speech-timeout";
    case kOutOfNetwork:
    case kTimedOut:
      return "network";
    case kPermissionDenied:
      return "not-allowed";
    case kRecorderBusy:
      return "audio-capture";
    case kInvalidLanguage:
      return "language-not-supported";
    case kNotSupported:
    case kNotSupportedFeature:
    case kEngineNotFound:
      return "service-not-allowed";
    case kServiceReset:
      return "interrupted";
    case kInvalidState:
    case kInProgressToReady:
    case kInProgressToRecording:
    case kInProgressToProcessing:
      return "busy";
    default:
      break;
  }
  // A Samsung TV reports silence as an operation failure: `no_speech` when the
  // mic button was released before a word, `reg_fail` when the remote sent no
  // sound at all (2024 The Frame, Tizen 9.0).
  if (message.find("no_speech") != std::string_view::npos || message.find("reg_fail") != std::string_view::npos) {
    return "no-speech";
  }
  return sttError == kOperationFailed ? "client" : "unknown";
}

} // namespace

SpeechError speechErrorFor(int sttError, std::string_view message) {
  const auto code = codeFor(sttError, message);
  return {.code = std::string(code), .message = message.empty() ? std::string(code) : std::string(message)};
}

} // namespace rntizen::expo::speech
