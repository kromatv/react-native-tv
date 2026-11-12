#pragma once

#include "SpeechContract.h"
#include "SpeechErrors.h"
#include "SttLibrary.h"

#include <folly/dynamic.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace rntizen::expo::speech {

/// One STT client and its session, reported as expo-speech-recognition's
/// events: `start`, `audiostart`, `speechstart`, `result`, `nomatch`, `error`,
/// `speechend`, `audioend` and, once per session, `end`. `start`, `stop` and
/// `abort` run on the main thread, where the library calls back; the readers
/// and `setEmit` may run on any thread.
class SpeechRecognizer final {
 public:
  using Emit = std::function<void(const std::string &event, folly::dynamic payload)>;
  using Defer = std::function<void(std::function<void()> task)>;

  /// Creates and prepares the client. `defer` runs a task once the library
  /// callback that queued it has returned: the library takes no call from its own callbacks.
  SpeechRecognizer(const SttLibrary &stt, Defer defer);

  SpeechRecognizer(const SpeechRecognizer &) = delete;
  SpeechRecognizer &operator=(const SpeechRecognizer &) = delete;

  void setEmit(Emit emit);
  bool usable() const;
  SpeechState state() const;
  /// The engine's languages in BCP 47, empty until the client is ready.
  std::vector<std::string> languages() const;

  void start(SpeechOptions options);
  /// Stops listening and keeps what was heard: a final `result` or `nomatch` follows.
  void stop();
  /// Drops the session with an `aborted` error.
  void abort();

 private:
  static void onState(SttHandle handle, int previous, int current, void *data);
  static void onResult(SttHandle handle, int event, const char **texts, int count, const char *message, void *data);
  static void onError(SttHandle handle, int reason, void *data);
  static void onSpeechStatus(SttHandle handle, int status, void *data);

  void becameReady();
  void begin();
  void recordingStarted();
  void finish();
  void emit(const std::string &event, folly::dynamic payload = nullptr) const;
  void emitError(const SpeechError &error) const;
  void emitAudio(const char *event) const;
  folly::dynamic results(const char **texts, int count) const;

  const SttLibrary &stt_;
  const Defer defer_;
  SttHandle handle_{nullptr};
  std::atomic<bool> usable_{false};
  std::atomic<SpeechState> state_{SpeechState::Inactive};
  bool clientReady_{false};
  bool startWhenReady_{false};
  bool stopWhenRecording_{false};
  bool audioStarted_{false};
  SpeechOptions options_;
  std::string defaultLanguage_;
  mutable std::mutex mutex_;
  std::vector<std::string> languages_;
  Emit emit_;
};

} // namespace rntizen::expo::speech
