#include "SpeechErrors.h"
#include "SttLibrary.h"

#include <doctest/doctest.h>

namespace rntizen::expo::speech {

TEST_CASE("an STT error is reported as the Web Speech error it stands for") {
  CHECK(speechErrorFor(stt_error::kNoSpeech, "").code == "no-speech");
  CHECK(speechErrorFor(stt_error::kRecordingTimedOut, "").code == "speech-timeout");
  CHECK(speechErrorFor(stt_error::kOutOfNetwork, "").code == "network");
  CHECK(speechErrorFor(stt_error::kTimedOut, "").code == "network");
  CHECK(speechErrorFor(stt_error::kPermissionDenied, "").code == "not-allowed");
  CHECK(speechErrorFor(stt_error::kRecorderBusy, "").code == "audio-capture");
  CHECK(speechErrorFor(stt_error::kInvalidLanguage, "").code == "language-not-supported");
  CHECK(speechErrorFor(stt_error::kEngineNotFound, "").code == "service-not-allowed");
  CHECK(speechErrorFor(stt_error::kNotSupported, "").code == "service-not-allowed");
  CHECK(speechErrorFor(stt_error::kNotSupportedFeature, "").code == "service-not-allowed");
  CHECK(speechErrorFor(stt_error::kServiceReset, "").code == "interrupted");
  CHECK(speechErrorFor(stt_error::kInvalidState, "").code == "busy");
  CHECK(speechErrorFor(stt_error::kInProgressToReady, "").code == "busy");
  CHECK(speechErrorFor(stt_error::kInProgressToRecording, "").code == "busy");
  CHECK(speechErrorFor(stt_error::kInProgressToProcessing, "").code == "busy");
  CHECK(speechErrorFor(stt_error::kOutOfMemory, "").code == "unknown");
}

TEST_CASE("an operation failure is told apart by the engine's message") {
  CHECK(speechErrorFor(stt_error::kOperationFailed, "voice_engine.error.no_speech").code == "no-speech");
  CHECK(speechErrorFor(stt_error::kOperationFailed, "voice_engine.error.reg_fail").code == "no-speech");
  const auto other = speechErrorFor(stt_error::kOperationFailed, "voice_engine.error.engine");
  CHECK(other.code == "client");
  CHECK(other.message == "voice_engine.error.engine");
  CHECK(speechErrorFor(stt_error::kOperationFailed, "").message == "client");
}

} // namespace rntizen::expo::speech
