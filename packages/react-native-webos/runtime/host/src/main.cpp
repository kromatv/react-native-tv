#include "ReactHost.h"
#include "intl/IcuData.h"
#include "logging/Log.h"
#include "platform/AppPaths.h"
#include "platform/LaunchParameters.h"
#include "platform/MainLoop.h"
#include "platform/TerminationSignals.h"

#include <SDL3/SDL.h>

#include <exception>
#include <memory>
#include <string>

int main(int argc, char **argv) {
  using namespace rnwebos;
  blockTerminationSignals();
  // Without the Back policy the TV keeps the key and closes the app itself
  // (docs/reference/webos-window-input.md, section 2).
  SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
  SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
  // The app manager keeps one registration per app, and the host's is the one
  // that reads what a relaunch carries; SDL's would also refuse every screen
  // saver (docs/reference/webos-window-input.md, section 15).
  SDL_SetHint(SDL_HINT_WEBOS_REGISTER_APP, "0");
  SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
  auto &loop = MainLoop::get();
  watchTerminationSignals([&loop] {
    loop.post([&loop] {
      log(LogLevel::Info, "Asked to end by a signal");
      loop.quit();
    });
  });
  const auto &resourceDir = appPaths().resource;
  useIcuData(resourceDir);
  std::unique_ptr<ReactHost> host;
  try {
    host = std::make_unique<ReactHost>(resourceDir, launchParametersOf(argc, argv));
  } catch (const std::exception &error) {
    log(LogLevel::Error, std::string("React Native failed to start: ") + error.what());
    SDL_Quit();
    return 1;
  }
  const int exitCode = loop.run();
  host.reset();
  SDL_Quit();
  return exitCode;
}
