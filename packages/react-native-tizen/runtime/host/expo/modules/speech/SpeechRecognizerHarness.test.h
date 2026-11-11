#pragma once

#include "SpeechRecognizer.h"

#include <cstring>
#include <deque>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace rntizen::expo::speech {

struct SttClient {};

/// A fake `libstt.so`: the recognizer's calls land in `fake`, and the tests
/// call its callbacks as the engine would.
struct FakeStt {
  SttLibrary::StateCallback onState = nullptr;
  SttLibrary::ResultCallback onResult = nullptr;
  SttLibrary::ErrorCallback onError = nullptr;
  SttLibrary::SpeechStatusCallback onSpeech = nullptr;
  void *data = nullptr;
  int createResult = stt_error::kNone;
  int startResult = stt_error::kNone;
  int stopResult = stt_error::kNone;
  int defaultLanguageResult = stt_error::kNone;
  int errorMessageResult = stt_error::kNone;
  std::vector<std::string> started;
  int stops = 0;
  int cancels = 0;
  std::string errorMessage;
  std::vector<std::string> languages{"en_US", "fr_FR", "de_DE"};
};

inline SttClient client;
inline FakeStt fake;

inline const SttLibrary kFakeLibrary{
    .create = [](SttHandle *handle) {
      *handle = &client;
      return fake.createResult;
    },
    .prepare = [](SttHandle) { return stt_error::kNone; },
    .start = [](SttHandle, const char *language, const char *) {
      fake.started.emplace_back(language);
      return fake.startResult;
    },
    .stop = [](SttHandle) {
      ++fake.stops;
      return fake.stopResult;
    },
    .cancel = [](SttHandle) {
      ++fake.cancels;
      return stt_error::kNone;
    },
    .getDefaultLanguage = [](SttHandle, char **language) {
      if (fake.defaultLanguageResult != stt_error::kNone) return fake.defaultLanguageResult;
      *language = strdup("fr_FR");
      return stt_error::kNone;
    },
    .foreachSupportedLanguage = [](SttHandle handle, SttLibrary::LanguageCallback callback, void *data) {
      for (const auto &language : fake.languages) {
        if (!callback(handle, language.c_str(), data)) break;
      }
      return stt_error::kNone;
    },
    .getErrorMessage = [](SttHandle, char **message) {
      if (fake.errorMessageResult != stt_error::kNone) return fake.errorMessageResult;
      *message = strdup(fake.errorMessage.c_str());
      return stt_error::kNone;
    },
    .setStateChangedCallback = [](SttHandle, SttLibrary::StateCallback callback, void *data) {
      fake.onState = callback;
      fake.data = data;
      return stt_error::kNone;
    },
    .setResultCallback = [](SttHandle, SttLibrary::ResultCallback callback, void *) {
      fake.onResult = callback;
      return stt_error::kNone;
    },
    .setErrorCallback = [](SttHandle, SttLibrary::ErrorCallback callback, void *) {
      fake.onError = callback;
      return stt_error::kNone;
    },
    .setSpeechStatusCallback = [](SttHandle, SttLibrary::SpeechStatusCallback callback, void *) {
      fake.onSpeech = callback;
      return stt_error::kNone;
    },
};

inline const SttLibrary &freshLibrary(int createResult = stt_error::kNone) {
  fake = FakeStt{};
  fake.createResult = createResult;
  return kFakeLibrary;
}

class Harness {
 public:
  explicit Harness(int createResult = stt_error::kNone)
      : recognizer(freshLibrary(createResult), [this](std::function<void()> task) { tasks_.push_back(std::move(task)); }) {
    recognizer.setEmit([this](const std::string &event, folly::dynamic payload) {
      events.emplace_back(event, std::move(payload));
    });
  }

  void state(SttState previous, SttState current) {
    fake.onState(&client, static_cast<int>(previous), static_cast<int>(current), fake.data);
    runTasks();
  }

  void ready() {
    state(SttState::Created, SttState::Ready);
  }

  void recording() {
    state(SttState::Ready, SttState::Recording);
  }

  void result(SttResultEvent event, std::vector<const char *> texts, const char *message = nullptr) {
    fake.onResult(&client, static_cast<int>(event), texts.data(), static_cast<int>(texts.size()), message, fake.data);
  }

  void error(int reason, std::string message) {
    fake.errorMessage = std::move(message);
    fake.onError(&client, reason, fake.data);
  }

  std::string names() const {
    std::string joined;
    for (const auto &[event, payload] : events) joined += (joined.empty() ? "" : ",") + event;
    return joined;
  }

  const folly::dynamic &last(const std::string &name) const {
    for (auto it = events.rbegin(); it != events.rend(); ++it) {
      if (it->first == name) return it->second;
    }
    throw std::runtime_error("no " + name + " event");
  }

  std::vector<std::pair<std::string, folly::dynamic>> events;
  SpeechRecognizer recognizer;

 private:
  void runTasks() {
    while (!tasks_.empty()) {
      auto task = std::move(tasks_.front());
      tasks_.pop_front();
      task();
    }
  }

  std::deque<std::function<void()>> tasks_;
};

} // namespace rntizen::expo::speech
