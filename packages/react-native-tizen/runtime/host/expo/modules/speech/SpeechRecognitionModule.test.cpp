#include "../../ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <stdexcept>
#include <string>

namespace rntizen::expo {

namespace {

ModuleContext withPrivilege(bool declared) {
  ModuleContext host;
  host.speechRecognition = declared;
  return host;
}

std::string errorsThenEnd(testing::ExpoTestRuntime &expo) {
  REQUIRE(expo.pumpUntil([&] { return expo.eval("events.length >= 2").getBool(); }));
  return expo.json("events.map(([name, payload]) => payload?.error ?? name)");
}

bool has(const std::string &json, const std::string &part) {
  return json.find(part) != std::string::npos;
}

void recordEvents(testing::ExpoTestRuntime &expo) {
  expo.eval(
      "globalThis.S = expo.modules.ExpoSpeechRecognition; globalThis.events = [];"
      "['start', 'result', 'error', 'end'].forEach(name => S.addListener(name, payload => events.push([name, payload])));");
}

} // namespace

TEST_CASE("ExpoSpeechRecognition on a TV without the engine reports it and ends a start") {
  testing::ExpoTestRuntime expo(withPrivilege(true));
  recordEvents(expo);

  expo.eval("S.start({ lang: 'fr-FR', interimResults: true })");

  CHECK(expo.json("S.isRecognitionAvailable()") == "false");
  CHECK(errorsThenEnd(expo) == R"(["service-not-allowed","end"])");
  CHECK(expo.settle("S.getStateAsync()") == "\"inactive\"");
  const auto locales = expo.settle("S.getSupportedLocales({})");
  CHECK(has(locales, R"("locales":[])"));
  CHECK(has(locales, R"("installedLocales":[])"));
  const auto permission = expo.settle("S.requestPermissionsAsync()");
  CHECK(has(permission, R"("status":"granted")"));
  CHECK(has(permission, R"("granted":true)"));
  CHECK(has(permission, R"("canAskAgain":false)"));
}

TEST_CASE("ExpoSpeechRecognition without the recorder privilege denies permission and ends a start") {
  testing::ExpoTestRuntime expo(withPrivilege(false));
  recordEvents(expo);

  expo.eval("S.start({})");

  CHECK(errorsThenEnd(expo) == R"(["not-allowed","end"])");
  CHECK(has(expo.settle("S.getPermissionsAsync()"), R"("granted":false)"));
  CHECK(has(expo.settle("S.getMicrophonePermissionsAsync()"), R"("status":"denied")"));
  CHECK(has(expo.settle("S.requestSpeechRecognizerPermissionsAsync()"), R"("status":"denied")"));
}

TEST_CASE("ExpoSpeechRecognition answers Android's and iOS's own questions as Android answers iOS's") {
  testing::ExpoTestRuntime expo(withPrivilege(true));
  expo.eval("globalThis.S = expo.modules.ExpoSpeechRecognition");

  expo.eval("S.stop(); S.abort(); S.setCategoryIOS({}); S.setAudioSessionActiveIOS(true)");

  CHECK(expo.json("[S.supportsOnDeviceRecognition(), S.supportsRecording(), S.getSpeechRecognitionServices()]") ==
        "[false,false,[]]");
  CHECK(expo.json("[S.getDefaultRecognitionService().packageName, S.getAssistantService().packageName]") ==
        R"(["",""])");
  CHECK(expo.json("S.getAudioSessionCategoryAndOptionsIOS().category") == R"("playAndRecord")");
  CHECK_THROWS_WITH_AS(
      expo.settle("S.androidTriggerOfflineModelDownload({ locale: 'fr-FR' })"),
      "not_supported: Offline speech models are an Android feature",
      std::runtime_error);
}

} // namespace rntizen::expo
