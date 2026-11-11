#include "../ExpoModuleRegistry.h"
#include "../ModuleDefinition.h"

#include "logging/Log.h"

#include <SDL3/SDL.h>

#include <set>

namespace rnwebos::expo {

namespace {

class ExpoKeepAwakeModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoKeepAwake")
        .asyncFunction("activate", [this](const std::string &tag) { return update(tag, true); }, Queue::Main)
        .asyncFunction("deactivate", [this](const std::string &tag) { return update(tag, false); }, Queue::Main)
        .asyncFunction("isActivated", [this] { return !tags_.empty(); }, Queue::Main)
        .asyncFunction("isAvailableAsync", [] { return SDL_WasInit(SDL_INIT_VIDEO) != 0; }, Queue::Main);
  }

 private:
  // SDL's screen saver switch is the one the host answers the TV's screen
  // saver requests from (platform/ScreenSaver.h). What SDL returns says only
  // whether its video driver did something more, which webOS's does not.
  bool update(const std::string &tag, bool active) {
    if (active) {
      tags_.insert(tag);
    } else {
      tags_.erase(tag);
    }
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
      log(LogLevel::Warning, "ExpoKeepAwake found no window to keep awake");
      return false;
    }
    if (tags_.empty()) {
      SDL_EnableScreenSaver();
    } else {
      SDL_DisableScreenSaver();
    }
    log(LogLevel::Debug,
        std::string("Keep awake ") + (SDL_ScreenSaverEnabled() ? "off" : "on") + " (" + std::to_string(tags_.size()) +
            " tags)");
    return true;
  }

  std::set<std::string, std::less<>> tags_;
};

} // namespace

RN_WEBOS_EXPO_MODULE(ExpoKeepAwake, ExpoKeepAwakeModule);

} // namespace rnwebos::expo
