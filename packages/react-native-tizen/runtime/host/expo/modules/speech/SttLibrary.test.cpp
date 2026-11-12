#include "SpeechRecognizerHarness.test.h"

#include <doctest/doctest.h>

#include <map>
#include <string>
#include <utility>

namespace rntizen::expo::speech {

namespace {

template <typename Function>
void *address(Function function) {
  return reinterpret_cast<void *>(function);
}

std::map<std::string, void *> symbolsOf(const SttLibrary &stt) {
  return {
      {"stt_create", address(stt.create)},
      {"stt_prepare", address(stt.prepare)},
      {"stt_start", address(stt.start)},
      {"stt_stop", address(stt.stop)},
      {"stt_cancel", address(stt.cancel)},
      {"stt_get_default_language", address(stt.getDefaultLanguage)},
      {"stt_foreach_supported_languages", address(stt.foreachSupportedLanguage)},
      {"stt_get_error_message", address(stt.getErrorMessage)},
      {"stt_set_state_changed_cb", address(stt.setStateChangedCallback)},
      {"stt_set_recognition_result_cb", address(stt.setResultCallback)},
      {"stt_set_error_cb", address(stt.setErrorCallback)},
      {"stt_set_speech_status_cb", address(stt.setSpeechStatusCallback)},
  };
}

SymbolLookup lookupIn(std::map<std::string, void *> symbols) {
  return [symbols = std::move(symbols)](const char *name) -> void * {
    const auto found = symbols.find(name);
    return found != symbols.end() ? found->second : nullptr;
  };
}

} // namespace

TEST_CASE("the library binds each function stt.h names") {
  const auto stt = loadSttLibrary(lookupIn(symbolsOf(kFakeLibrary)));

  REQUIRE(stt.has_value());
  CHECK((stt->create == kFakeLibrary.create));
  CHECK((stt->setErrorCallback == kFakeLibrary.setErrorCallback));
  CHECK((stt->setSpeechStatusCallback == kFakeLibrary.setSpeechStatusCallback));
}

TEST_CASE("a library from before Tizen 5.5 binds without the speech status callback") {
  auto symbols = symbolsOf(kFakeLibrary);
  symbols.erase("stt_set_speech_status_cb");

  const auto stt = loadSttLibrary(lookupIn(std::move(symbols)));

  REQUIRE(stt.has_value());
  CHECK((stt->setSpeechStatusCallback == nullptr));
}

TEST_CASE("a library missing a function the recognizer calls is not used") {
  auto symbols = symbolsOf(kFakeLibrary);
  symbols.erase("stt_cancel");

  CHECK_FALSE(loadSttLibrary(lookupIn(std::move(symbols))).has_value());
}

} // namespace rntizen::expo::speech
