#include "SpeechRecognizerHarness.test.h"

#include <doctest/doctest.h>

namespace rntizen::expo::speech {

TEST_CASE("abort drops the session with an aborted error and ends it once") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});
  speech.recording();

  speech.recognizer.abort();
  speech.state(SttState::Recording, SttState::Ready);
  speech.result(SttResultEvent::Final, {"late"});

  CHECK(fake.cancels == 1);
  CHECK(speech.names() == "start,audiostart,error,audioend,end");
  CHECK(speech.last("error")["error"].getString() == "aborted");
}

TEST_CASE("silence the engine reports as an operation failure is no-speech") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});
  speech.recording();

  speech.error(stt_error::kOperationFailed, "voice_engine.error.reg_fail");
  speech.state(SttState::Recording, SttState::Ready);

  CHECK(speech.names() == "start,audiostart,error,audioend,end");
  CHECK(speech.last("error")["error"].getString() == "no-speech");
  CHECK(speech.last("error")["message"].getString() == "voice_engine.error.reg_fail");
}

TEST_CASE("an error before the recording starts ends the session at once") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});

  speech.error(stt_error::kRecorderBusy, "");

  CHECK(speech.names() == "error,end");
  CHECK(speech.last("error")["error"].getString() == "audio-capture");
}

TEST_CASE("a start the engine refuses ends the session with its error") {
  Harness speech;
  speech.ready();
  fake.startResult = stt_error::kPermissionDenied;

  speech.recognizer.start({});

  CHECK(speech.names() == "error,end");
  CHECK(speech.last("error")["error"].getString() == "not-allowed");
}

TEST_CASE("a final result with nothing heard is nomatch, and the engine's speech marks are reported") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});
  speech.recording();

  fake.onSpeech(&client, static_cast<int>(SttSpeechStatus::Beginning), fake.data);
  fake.onSpeech(&client, static_cast<int>(SttSpeechStatus::End), fake.data);
  speech.result(SttResultEvent::Final, {""});
  speech.result(SttResultEvent::Error, {}, "voice_engine.error.no_speech");

  CHECK(speech.names() == "start,audiostart,speechstart,speechend,nomatch,error");
  CHECK(speech.last("error")["error"].getString() == "no-speech");
}

TEST_CASE("a recording nobody asked for is cancelled") {
  Harness speech;
  speech.ready();

  speech.recording();

  CHECK(fake.cancels == 1);
  CHECK(speech.events.empty());
}

TEST_CASE("a client that cannot be made is unusable, and a start ends at once") {
  Harness speech(stt_error::kNotSupported);

  speech.recognizer.start({});

  CHECK_FALSE(speech.recognizer.usable());
  CHECK(speech.names() == "error,end");
  CHECK(speech.last("error")["error"].getString() == "service-not-allowed");
}

TEST_CASE("an error before the client is ready leaves it unusable") {
  Harness speech;

  speech.error(stt_error::kEngineNotFound, "");

  CHECK_FALSE(speech.recognizer.usable());
  CHECK(speech.events.empty());
}

TEST_CASE("a stop the engine refuses cancels the recording and ends the session with its error") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});
  speech.recording();
  fake.stopResult = stt_error::kInvalidState;

  speech.recognizer.stop();

  CHECK(fake.cancels == 1);
  CHECK(speech.names() == "start,audiostart,error,audioend,end");
  CHECK(speech.last("error")["error"].getString() == "busy");
}

TEST_CASE("an engine that gives no message, or no default language, still reports and listens") {
  Harness speech;
  fake.defaultLanguageResult = stt_error::kOperationFailed;
  fake.errorMessageResult = stt_error::kOperationFailed;
  speech.ready();
  speech.recognizer.start({.language = "en-US"});
  speech.recording();

  speech.result(SttResultEvent::Error, {});
  speech.error(stt_error::kRecorderBusy, "ignored");

  CHECK(fake.started == std::vector<std::string>{"en_US"});
  CHECK(speech.events[2].second["message"].getString() == "client");
  CHECK(speech.last("error")["message"].getString() == "audio-capture");
}

TEST_CASE("the engine's callbacks with no session running are ignored") {
  Harness speech;
  speech.ready();

  fake.onSpeech(&client, static_cast<int>(SttSpeechStatus::Beginning), fake.data);
  speech.error(stt_error::kServiceReset, "");

  CHECK(speech.recognizer.usable());
  CHECK(speech.events.empty());
}

} // namespace rntizen::expo::speech
