#include "SpeechRecognizerHarness.test.h"

#include <doctest/doctest.h>

namespace rntizen::expo::speech {

TEST_CASE("a session reports its start, partial and final results and its end, in the TV's language") {
  Harness speech;
  speech.ready();

  speech.recognizer.start({.interimResults = true});
  speech.recording();
  speech.result(SttResultEvent::Partial, {"du"});
  speech.recognizer.stop();
  speech.state(SttState::Recording, SttState::Processing);
  speech.result(SttResultEvent::Final, {"dune"});
  speech.state(SttState::Processing, SttState::Ready);

  CHECK(fake.started == std::vector<std::string>{"fr_FR"});
  CHECK(fake.stops == 1);
  CHECK(speech.names() == "start,audiostart,result,audioend,result,end");
  CHECK(speech.last("result")["isFinal"].getBool());
  CHECK(speech.last("result")["results"][0]["transcript"].getString() == "dune");
  CHECK(speech.events[2].second["isFinal"].getBool() == false);
  CHECK(speech.recognizer.state() == SpeechState::Inactive);
}

TEST_CASE("partial results stay out unless interimResults asks for them") {
  Harness speech;
  speech.ready();

  speech.recognizer.start({});
  speech.recording();
  speech.result(SttResultEvent::Partial, {"du"});
  speech.result(SttResultEvent::Final, {"dune", "dunes"});
  speech.state(SttState::Recording, SttState::Ready);

  CHECK(speech.names() == "start,audiostart,result,audioend,end");
  CHECK(speech.last("result")["results"].size() == 1);
}

TEST_CASE("a start before the client is ready waits for it") {
  Harness speech;

  speech.recognizer.start({.language = "en-US"});
  CHECK(fake.started.empty());
  CHECK(speech.recognizer.state() == SpeechState::Starting);
  speech.ready();

  CHECK(fake.started == std::vector<std::string>{"en_US"});
  CHECK(speech.recognizer.languages() == std::vector<std::string>{"en-US", "fr-FR", "de-DE"});
}

TEST_CASE("a language is taken in the engine's spelling, and one it lacks ends the session") {
  Harness speech;
  speech.ready();

  speech.recognizer.start({.language = "de"});
  speech.recording();
  speech.state(SttState::Recording, SttState::Ready);
  speech.recognizer.start({.language = "ja-JP"});

  CHECK(fake.started == std::vector<std::string>{"de_DE"});
  CHECK(speech.last("error")["error"].getString() == "language-not-supported");
  CHECK(speech.names().ends_with("error,end"));
}

TEST_CASE("a stop before the recording starts stops it as soon as it does") {
  Harness speech;
  speech.ready();

  speech.recognizer.start({});
  speech.recognizer.stop();
  CHECK(fake.stops == 0);
  speech.recording();

  CHECK(fake.stops == 1);
  CHECK(speech.recognizer.state() == SpeechState::Stopping);
}

TEST_CASE("a stop while the client is still preparing ends the session there") {
  Harness speech;

  speech.recognizer.start({});
  speech.recognizer.stop();
  speech.ready();

  CHECK(fake.started.empty());
  CHECK(speech.names() == "end");
}

TEST_CASE("a second start while listening changes nothing") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});
  speech.recording();

  speech.recognizer.start({.language = "en-US"});

  CHECK(fake.started == std::vector<std::string>{"fr_FR"});
}

TEST_CASE("a stop or an abort with no session changes nothing") {
  Harness speech;
  speech.ready();

  speech.recognizer.stop();
  speech.recognizer.abort();

  CHECK(fake.stops == 0);
  CHECK(fake.cancels == 0);
  CHECK(speech.events.empty());
}

TEST_CASE("an abort while the client is preparing ends the session without a recording to cancel") {
  Harness speech;
  speech.recognizer.start({});

  speech.recognizer.abort();
  speech.ready();

  CHECK(fake.cancels == 0);
  CHECK(fake.started.empty());
  CHECK(speech.names() == "error,end");
}

TEST_CASE("the engine processing on its own moves the session to stopping") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({});
  speech.recording();

  speech.state(SttState::Recording, SttState::Processing);

  CHECK(speech.recognizer.state() == SpeechState::Stopping);
  CHECK(speech.names() == "start,audiostart,audioend");
}

TEST_CASE("a client prepared again, or unprepared, ends no session") {
  Harness speech;
  speech.ready();

  speech.ready();
  speech.state(SttState::Ready, SttState::Created);

  CHECK(speech.events.empty());
}

TEST_CASE("interim results with no text are dropped") {
  Harness speech;
  speech.ready();
  speech.recognizer.start({.interimResults = true});
  speech.recording();

  speech.result(SttResultEvent::Partial, {""});
  speech.result(SttResultEvent::Partial, {nullptr, "du"});

  CHECK(speech.names() == "start,audiostart,result");
  CHECK(speech.last("result")["results"][0]["transcript"].getString() == "du");
}

TEST_CASE("a library from before Tizen 5.5 runs without speech marks, and nobody listening hears nothing") {
  auto old = freshLibrary();
  old.setSpeechStatusCallback = nullptr;
  SpeechRecognizer recognizer(old, [](std::function<void()> task) { task(); });

  fake.onState(&client, static_cast<int>(SttState::Created), static_cast<int>(SttState::Ready), fake.data);
  recognizer.start({});

  CHECK((fake.onSpeech == nullptr));
  CHECK(fake.started == std::vector<std::string>{"fr_FR"});
}

} // namespace rntizen::expo::speech
