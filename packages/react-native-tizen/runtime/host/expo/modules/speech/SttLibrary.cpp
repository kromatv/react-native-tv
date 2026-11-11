#include "SttLibrary.h"

#include <dlfcn.h>

#include <algorithm>
#include <array>
#include <functional>

namespace rntizen::expo::speech {

namespace {

template <typename Lookup, typename Function>
bool bind(const Lookup &lookup, const char *name, Function &slot) {
  slot = reinterpret_cast<Function>(lookup(name));
  return slot != nullptr;
}

template <typename Lookup>
bool bindRequired(const Lookup &lookup, SttLibrary &stt) {
  const std::array bound{
      bind(lookup, "stt_create", stt.create),
      bind(lookup, "stt_prepare", stt.prepare),
      bind(lookup, "stt_start", stt.start),
      bind(lookup, "stt_stop", stt.stop),
      bind(lookup, "stt_cancel", stt.cancel),
      bind(lookup, "stt_get_default_language", stt.getDefaultLanguage),
      bind(lookup, "stt_foreach_supported_languages", stt.foreachSupportedLanguage),
      bind(lookup, "stt_get_error_message", stt.getErrorMessage),
      bind(lookup, "stt_set_state_changed_cb", stt.setStateChangedCallback),
      bind(lookup, "stt_set_recognition_result_cb", stt.setResultCallback),
      bind(lookup, "stt_set_error_cb", stt.setErrorCallback),
  };
  return std::ranges::all_of(bound, std::identity{});
}

std::optional<SttLibrary> open() {
  // Opened at run time, so a TV without speech recognition still loads the
  // runtime and the simulator needs no stand-in for it.
  void *library = dlopen("libstt.so", RTLD_LAZY | RTLD_LOCAL);
  if (library == nullptr) return std::nullopt;
  return loadSttLibrary([library](const char *name) { return dlsym(library, name); });
}

} // namespace

std::optional<SttLibrary> loadSttLibrary(const SymbolLookup &lookup) {
  SttLibrary stt{};
  if (!bindRequired(lookup, stt)) return std::nullopt;
  bind(lookup, "stt_set_speech_status_cb", stt.setSpeechStatusCallback);
  return stt;
}

const SttLibrary *sttLibrary() {
  static const std::optional<SttLibrary> library = open();
  return library ? &*library : nullptr;
}

} // namespace rntizen::expo::speech
