#pragma once

#include <functional>
#include <optional>

namespace rntizen::expo::speech {

/// An `stt_h`. The library's types are mirrored from Tizen's `stt.h`, unchanged
/// since 2.3, so the runtime compiles where the header is absent (the simulator).
using SttHandle = struct SttClient *;

/// `stt_state_e`.
enum class SttState : int { Created = 0, Ready = 1, Recording = 2, Processing = 3 };

/// `stt_result_event_e`.
enum class SttResultEvent : int { Final = 0, Partial = 1, Error = 2 };

/// `stt_speech_status_e`, since Tizen 5.5.
enum class SttSpeechStatus : int { Beginning = 0, End = 1 };

/// `stt_error_e`: `TIZEN_ERROR_STT` is `-0x02F00000` and the shared codes are
/// `tizen_error.h`'s, read from the Tizen 6.0 rootstrap.
namespace stt_error {
inline constexpr int kNone = 0;
inline constexpr int kOutOfMemory = -12;
inline constexpr int kIoError = -5;
inline constexpr int kInvalidParameter = -22;
inline constexpr int kTimedOut = -1073741823;
inline constexpr int kRecorderBusy = -16;
inline constexpr int kOutOfNetwork = -100;
inline constexpr int kPermissionDenied = -13;
inline constexpr int kNotSupported = -1073741822;
inline constexpr int kSttBase = -0x02F00000;
inline constexpr int kInvalidState = kSttBase | 0x01;
inline constexpr int kInvalidLanguage = kSttBase | 0x02;
inline constexpr int kEngineNotFound = kSttBase | 0x03;
inline constexpr int kOperationFailed = kSttBase | 0x04;
inline constexpr int kNotSupportedFeature = kSttBase | 0x05;
inline constexpr int kRecordingTimedOut = kSttBase | 0x06;
inline constexpr int kNoSpeech = kSttBase | 0x07;
inline constexpr int kInProgressToReady = kSttBase | 0x08;
inline constexpr int kInProgressToRecording = kSttBase | 0x09;
inline constexpr int kInProgressToProcessing = kSttBase | 0x10;
inline constexpr int kServiceReset = kSttBase | 0x11;
} // namespace stt_error

/// `STT_RECOGNITION_TYPE_FREE`: free dictation, which also reports partial results.
inline constexpr const char *kFreeDictation = "stt.recognition.type.FREE";

/// The functions of `libstt.so` the recognizer calls, with `stt.h`'s signatures.
struct SttLibrary {
  using StateCallback = void (*)(SttHandle, int previous, int current, void *data);
  using ResultCallback =
      void (*)(SttHandle, int event, const char **texts, int count, const char *message, void *data);
  using ErrorCallback = void (*)(SttHandle, int reason, void *data);
  using SpeechStatusCallback = void (*)(SttHandle, int status, void *data);
  using LanguageCallback = bool (*)(SttHandle, const char *language, void *data);

  int (*create)(SttHandle *handle);
  int (*prepare)(SttHandle handle);
  int (*start)(SttHandle handle, const char *language, const char *type);
  int (*stop)(SttHandle handle);
  int (*cancel)(SttHandle handle);
  int (*getDefaultLanguage)(SttHandle handle, char **language);
  int (*foreachSupportedLanguage)(SttHandle handle, LanguageCallback callback, void *data);
  int (*getErrorMessage)(SttHandle handle, char **message);
  int (*setStateChangedCallback)(SttHandle handle, StateCallback callback, void *data);
  int (*setResultCallback)(SttHandle handle, ResultCallback callback, void *data);
  int (*setErrorCallback)(SttHandle handle, ErrorCallback callback, void *data);
  /// Null before Tizen 5.5.
  int (*setSpeechStatusCallback)(SttHandle handle, SpeechStatusCallback callback, void *data);
};

/// Finds one of the library's functions by name, or null: `dlsym` on the opened library.
using SymbolLookup = std::function<void *(const char *name)>;

/// Binds `stt.h`'s functions through `lookup`; empty when one the recognizer needs is missing.
std::optional<SttLibrary> loadSttLibrary(const SymbolLookup &lookup);

/// Tizen's speech-to-text library, opened once; null where the TV or the simulator has none.
const SttLibrary *sttLibrary();

} // namespace rntizen::expo::speech
