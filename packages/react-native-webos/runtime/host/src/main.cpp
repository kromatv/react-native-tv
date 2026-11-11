#include "ReactHost.h"
#include "intl/IcuData.h"
#include "logging/Log.h"
#include "platform/AppPaths.h"
#include "platform/MainLoop.h"

#include <SDL3/SDL.h>

#include <exception>
#include <memory>
#include <string>

int main(int, char **) {
  using namespace rnwebos;
  // Without the Back policy the TV keeps the key and closes the app itself
  // (docs/reference/webos-window-input.md, section 2).
  SDL_SetHint(SDL_HINT_WEBOS_ACCESS_POLICY_KEYS_BACK, "true");
  SDL_SetHint(SDL_HINT_WEBOS_CURSOR_SLEEP_TIME, "5000");
  auto &loop = MainLoop::get();
  const auto &resourceDir = appPaths().resource;
  useIcuData(resourceDir);
  std::unique_ptr<ReactHost> host;
  try {
    host = std::make_unique<ReactHost>(resourceDir);
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
