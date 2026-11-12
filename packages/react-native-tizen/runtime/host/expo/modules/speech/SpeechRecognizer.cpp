#include "SpeechRecognizer.h"

#include "SpeechLocale.h"

#include <chrono>
#include <cstdlib>
#include <utility>

namespace rntizen::expo::speech {

namespace {

SpeechRecognizer &self(void *data) {
  return *static_cast<SpeechRecognizer *>(data);
}

std::string taken(char *text) {
  std::string value = text != nullptr ? text : "";
  std::free(text);
  return value;
}

double nowMillis() {
  return static_cast<double>(
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count());
}

bool collectLanguage(SttHandle, const char *language, void *data) {
  if (language != nullptr) static_cast<std::vector<std::string> *>(data)->emplace_back(language);
  return true;
}

} // namespace

SpeechRecognizer::SpeechRecognizer(const SttLibrary &stt, Defer defer) : stt_(stt), defer_(std::move(defer)) {
  if (stt_.create(&handle_) != stt_error::kNone) {
    handle_ = nullptr;
    return;
  }
  stt_.setStateChangedCallback(handle_, onState, this);
  stt_.setResultCallback(handle_, onResult, this);
  stt_.setErrorCallback(handle_, onError, this);
  if (stt_.setSpeechStatusCallback != nullptr) stt_.setSpeechStatusCallback(handle_, onSpeechStatus, this);
  usable_ = stt_.prepare(handle_) == stt_error::kNone;
}

void SpeechRecognizer::setEmit(Emit emit) {
  std::scoped_lock lock(mutex_);
  emit_ = std::move(emit);
}

bool SpeechRecognizer::usable() const {
  return usable_;
}

SpeechState SpeechRecognizer::state() const {
  return state_;
}

std::vector<std::string> SpeechRecognizer::languages() const {
  std::scoped_lock lock(mutex_);
  std::vector<std::string> tags;
  tags.reserve(languages_.size());
  for (const auto &language : languages_) tags.push_back(toBcp47(language));
  return tags;
}

void SpeechRecognizer::start(SpeechOptions options) {
  if (state_ != SpeechState::Inactive) return;
  if (!usable_) {
    emitError({.code = "service-not-allowed", .message = "Speech recognition is not available on this TV"});
    emit("end");
    return;
  }
  options_ = std::move(options);
  stopWhenRecording_ = false;
  state_ = SpeechState::Starting;
  if (clientReady_) {
    begin();
  } else {
    startWhenReady_ = true;
  }
}

void SpeechRecognizer::stop() {
  using enum SpeechState;
  if (state_ == Starting) {
    if (startWhenReady_) {
      startWhenReady_ = false;
      finish();
    } else {
      stopWhenRecording_ = true;
    }
    return;
  }
  if (state_ != Recognizing) return;
  state_ = Stopping;
  if (const int error = stt_.stop(handle_); error != stt_error::kNone) {
    emitError(speechErrorFor(error, ""));
    stt_.cancel(handle_);
    finish();
  }
}

void SpeechRecognizer::abort() {
  if (state_ == SpeechState::Inactive) return;
  emitError({.code = "aborted", .message = "Speech recognition aborted."});
  const bool listening = !startWhenReady_;
  startWhenReady_ = false;
  if (listening) stt_.cancel(handle_);
  finish();
}

void SpeechRecognizer::becameReady() {
  std::vector<std::string> languages;
  stt_.foreachSupportedLanguage(handle_, collectLanguage, &languages);
  char *language = nullptr;
  defaultLanguage_ = stt_.getDefaultLanguage(handle_, &language) == stt_error::kNone ? taken(language) : "";
  {
    std::scoped_lock lock(mutex_);
    languages_ = std::move(languages);
  }
  clientReady_ = true;
  if (!startWhenReady_) return;
  startWhenReady_ = false;
  begin();
}

void SpeechRecognizer::begin() {
  std::vector<std::string> supported;
  {
    std::scoped_lock lock(mutex_);
    supported = languages_;
  }
  const auto language = sttLanguageFor(options_.language, supported, defaultLanguage_);
  if (!language) {
    emitError({.code = "language-not-supported", .message = "This TV does not recognize " + options_.language});
    finish();
    return;
  }
  if (const int error = stt_.start(handle_, language->c_str(), kFreeDictation); error != stt_error::kNone) {
    emitError(speechErrorFor(error, ""));
    finish();
  }
}

void SpeechRecognizer::recordingStarted() {
  if (state_ != SpeechState::Starting) {
    stt_.cancel(handle_);
    return;
  }
  state_ = SpeechState::Recognizing;
  emit("start");
  audioStarted_ = true;
  emitAudio("audiostart");
  if (stopWhenRecording_) stop();
}

void SpeechRecognizer::finish() {
  if (state_ == SpeechState::Inactive) return;
  if (audioStarted_) {
    audioStarted_ = false;
    emitAudio("audioend");
  }
  state_ = SpeechState::Inactive;
  emit("end");
}

void SpeechRecognizer::onState(SttHandle, int previous, int current, void *data) {
  auto &recognizer = self(data);
  switch (static_cast<SttState>(current)) {
    case SttState::Ready:
      if (!recognizer.clientReady_) {
        recognizer.defer_([&recognizer] { recognizer.becameReady(); });
      } else if (static_cast<SttState>(previous) != SttState::Created) {
        recognizer.finish();
      }
      return;
    case SttState::Recording:
      recognizer.defer_([&recognizer] { recognizer.recordingStarted(); });
      return;
    case SttState::Processing:
      if (recognizer.audioStarted_) {
        recognizer.audioStarted_ = false;
        recognizer.emitAudio("audioend");
      }
      if (recognizer.state_ == SpeechState::Recognizing) recognizer.state_ = SpeechState::Stopping;
      return;
    case SttState::Created:
      return;
  }
}

void SpeechRecognizer::onResult(SttHandle, int event, const char **texts, int count, const char *message, void *data) {
  const auto &recognizer = self(data);
  if (recognizer.state_ == SpeechState::Inactive) return;
  using enum SttResultEvent;
  switch (static_cast<SttResultEvent>(event)) {
    case Partial:
      if (auto found = recognizer.results(texts, count); recognizer.options_.interimResults && !found.empty()) {
        recognizer.emit("result", folly::dynamic::object("isFinal", false)("results", std::move(found)));
      }
      return;
    case Final:
      if (auto found = recognizer.results(texts, count); found.empty()) {
        recognizer.emit("nomatch");
      } else {
        recognizer.emit("result", folly::dynamic::object("isFinal", true)("results", std::move(found)));
      }
      return;
    case Error:
      recognizer.emitError(speechErrorFor(stt_error::kOperationFailed, message != nullptr ? message : ""));
      return;
  }
}

void SpeechRecognizer::onError(SttHandle handle, int reason, void *data) {
  auto &recognizer = self(data);
  char *message = nullptr;
  const auto why = recognizer.stt_.getErrorMessage(handle, &message) == stt_error::kNone ? taken(message) : "";
  if (!recognizer.clientReady_) recognizer.usable_ = false;
  if (recognizer.state_ == SpeechState::Inactive) return;
  recognizer.emitError(speechErrorFor(reason, why));
  if (recognizer.state_ == SpeechState::Starting) {
    recognizer.startWhenReady_ = false;
    recognizer.finish();
  }
}

void SpeechRecognizer::onSpeechStatus(SttHandle, int status, void *data) {
  const auto &recognizer = self(data);
  if (recognizer.state_ == SpeechState::Inactive) return;
  recognizer.emit(static_cast<SttSpeechStatus>(status) == SttSpeechStatus::Beginning ? "speechstart" : "speechend");
}

void SpeechRecognizer::emit(const std::string &event, folly::dynamic payload) const {
  std::scoped_lock lock(mutex_);
  if (emit_) emit_(event, std::move(payload));
}

void SpeechRecognizer::emitError(const SpeechError &error) const {
  emit("error", folly::dynamic::object("error", error.code)("message", error.message));
}

void SpeechRecognizer::emitAudio(const char *event) const {
  emit(event, folly::dynamic::object("uri", nullptr)("timestamp", nowMillis()));
}

folly::dynamic SpeechRecognizer::results(const char **texts, int count) const {
  folly::dynamic found = folly::dynamic::array();
  for (int index = 0; texts != nullptr && index < count && found.size() < static_cast<std::size_t>(options_.maxAlternatives);
       ++index) {
    if (texts[index] == nullptr || texts[index][0] == '\0') continue;
    found.push_back(
        folly::dynamic::object("transcript", texts[index])("confidence", 0)("segments", folly::dynamic::array()));
  }
  return found;
}

} // namespace rntizen::expo::speech
