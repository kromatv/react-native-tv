#include "SpeechRecognizer.h"

#include "../../ExpoModuleRegistry.h"
#include "../../ModuleDefinition.h"
#include "threading/MainThread.h"

#include <atomic>
#include <memory>
#include <mutex>

namespace rntizen::expo {

namespace {

using speech::SpeechOptions;
using speech::SpeechRecognizer;
using speech::SpeechState;

folly::dynamic permission(bool granted) {
  return folly::dynamic::object("status", granted ? "granted" : "denied")("granted", granted)("canAskAgain", false)(
      "expires", "never");
}

folly::dynamic failure(const char *code, const char *message) {
  return folly::dynamic::object("error", code)("message", message);
}

class SpeechRecognitionModule final : public Module {
 public:
  SpeechRecognitionModule() = default;
  SpeechRecognitionModule(const SpeechRecognitionModule &) = delete;
  SpeechRecognitionModule &operator=(const SpeechRecognitionModule &) = delete;

  ~SpeechRecognitionModule() override {
    std::scoped_lock lock(currentMutex_);
    if (current_ == this) current_ = nullptr;
  }

  ModuleDefinition definition() override {
    {
      std::scoped_lock lock(currentMutex_);
      current_ = this;
    }
    runOnMainThread([] { recognizer(); });
    return ModuleDefinition("ExpoSpeechRecognition")
        .function("start", [this](const folly::dynamic &options) { start(speech::speechOptionsFrom(options)); })
        .function("stop", [] { onRecognizer([](SpeechRecognizer &speech) { speech.stop(); }); })
        .function("abort", [] { onRecognizer([](SpeechRecognizer &speech) { speech.abort(); }); })
        .function("isRecognitionAvailable", [this] { return available(); })
        .function("supportsOnDeviceRecognition", [] { return false; })
        .function("supportsRecording", [] { return false; })
        .function("getSpeechRecognitionServices", [] { return std::vector<std::string>{}; })
        .function("getDefaultRecognitionService", []() -> folly::dynamic { return folly::dynamic::object("packageName", ""); })
        .function("getAssistantService", []() -> folly::dynamic { return folly::dynamic::object("packageName", ""); })
        .function("setCategoryIOS", [](const folly::dynamic &) {})
        .function("setAudioSessionActiveIOS", [](const folly::dynamic &) {})
        .function(
            "getAudioSessionCategoryAndOptionsIOS",
            []() -> folly::dynamic {
              return folly::dynamic::object("category", "playAndRecord")(
                  "categoryOptions", folly::dynamic::array("defaultToSpeaker", "allowBluetooth"))("mode", "measurement");
            })
        .asyncFunction("getSupportedLocales", [](const folly::dynamic &) { return supportedLocales(); })
        .asyncFunction("getStateAsync", [] { return std::string(speech::speechStateName(currentState())); })
        .asyncFunction("requestPermissionsAsync", [this] { return permission(configured()); })
        .asyncFunction("getPermissionsAsync", [this] { return permission(configured()); })
        .asyncFunction("requestMicrophonePermissionsAsync", [this] { return permission(configured()); })
        .asyncFunction("getMicrophonePermissionsAsync", [this] { return permission(configured()); })
        .asyncFunction("requestSpeechRecognizerPermissionsAsync", [this] { return permission(configured()); })
        .asyncFunction("getSpeechRecognizerPermissionsAsync", [this] { return permission(configured()); })
        .asyncFunction("androidTriggerOfflineModelDownload", [](const folly::dynamic &, const Promise &promise) {
          promise.reject("not_supported", "Offline speech models are an Android feature");
        });
  }

 private:
  // One client for the process, made on the main thread, where the library
  // calls back, and kept across reloads: a reload must not free what a
  // callback still points to. Events go to the module JavaScript holds now.
  static SpeechRecognizer *recognizer() {
    static const std::unique_ptr<SpeechRecognizer> made = [] {
      const auto *library = speech::sttLibrary();
      if (library == nullptr) return std::unique_ptr<SpeechRecognizer>();
      auto client = std::make_unique<SpeechRecognizer>(*library, [](std::function<void()> task) {
        runOnMainThread(std::move(task));
      });
      client->setEmit(route);
      shared_ = client.get();
      return client;
    }();
    return made.get();
  }

  static void route(const std::string &event, folly::dynamic payload) {
    std::scoped_lock lock(currentMutex_);
    if (current_ != nullptr) current_->sendEvent(event, std::move(payload));
  }

  static void onRecognizer(std::function<void(SpeechRecognizer &)> action) {
    runOnMainThread([action = std::move(action)] {
      if (auto *speech = recognizer(); speech != nullptr) action(*speech);
    });
  }

  static SpeechState currentState() {
    const auto *speech = shared_.load();
    return speech != nullptr ? speech->state() : SpeechState::Inactive;
  }

  static folly::dynamic supportedLocales() {
    folly::dynamic locales = folly::dynamic::array();
    if (const auto *speech = shared_.load(); speech != nullptr) {
      for (const auto &language : speech->languages()) locales.push_back(language);
    }
    return folly::dynamic::object("locales", locales)("installedLocales", folly::dynamic::array());
  }

  bool configured() const {
    return context().host().speechRecognition;
  }

  bool available() const {
    if (!configured() || speech::sttLibrary() == nullptr) return false;
    const auto *speech = shared_.load();
    return speech == nullptr || speech->usable();
  }

  void start(SpeechOptions options) const {
    if (!configured()) {
      sendEvent(
          "error",
          failure("not-allowed", "The package lacks the recorder privilege: add expo-speech-recognition to the plugins"));
      sendEvent("end", nullptr);
      return;
    }
    if (speech::sttLibrary() == nullptr) {
      sendEvent("error", failure("service-not-allowed", "This TV has no speech recognition"));
      sendEvent("end", nullptr);
      return;
    }
    onRecognizer([options = std::move(options)](SpeechRecognizer &speech) mutable { speech.start(std::move(options)); });
  }

  static inline std::atomic<SpeechRecognizer *> shared_{nullptr};
  static inline std::mutex currentMutex_;
  static inline SpeechRecognitionModule *current_{nullptr};
};

} // namespace

} // namespace rntizen::expo

RN_TIZEN_EXPO_MODULE(ExpoSpeechRecognition, rntizen::expo::SpeechRecognitionModule);
