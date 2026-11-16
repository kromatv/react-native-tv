#include "AcbLibrary.h"

#include "../logging/Log.h"

#include <dlfcn.h>

#include <bit>

namespace rnwebos {

namespace {

template <typename Function>
bool bind(void *library, const char *name, Function &function) {
  function = std::bit_cast<Function>(dlsym(library, name));
  return function != nullptr;
}

const AcbApi *loadApi() {
  // Opened, never linked: a DT_NEEDED on a library webOS 5 deleted would stop the executable before main.
  void *library = dlopen("libAcbAPI.so.1", RTLD_NOW | RTLD_LOCAL);
  if (library == nullptr) return nullptr;
  static AcbApi api;
  if (const bool complete = bind(library, "AcbAPI_create", api.create) &&
                            bind(library, "AcbAPI_initialize", api.initialize) &&
                            bind(library, "AcbAPI_setSinkType", api.setSinkType) &&
                            bind(library, "AcbAPI_setMediaId", api.setMediaId) &&
                            bind(library, "AcbAPI_setState", api.setState) &&
                            bind(library, "AcbAPI_setMediaVideoData", api.setMediaVideoData) &&
                            bind(library, "AcbAPI_setDisplayWindow", api.setDisplayWindow);
      !complete) {
    log(LogLevel::Warning, "libAcbAPI.so.1 lacks a call the video plane needs: no picture on this TV");
    return nullptr;
  }
  log(LogLevel::Info, "libAcbAPI.so.1 is here: video goes on the plane through the control block (webOS 4)");
  return &api;
}

} // namespace

const AcbApi *acbApi() {
  static const AcbApi *const api = loadApi();
  return api;
}

} // namespace rnwebos
