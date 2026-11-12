#include "../ExpoTestRuntime.test.h"

#include <SDL3/SDL.h>
#include <doctest/doctest.h>

namespace rnwebos::expo {

TEST_CASE("keep awake disables SDL's screen saver while any tag is active, and enables it after the last") {
  REQUIRE(SDL_InitSubSystem(SDL_INIT_VIDEO));
  SDL_EnableScreenSaver();
  testing::ExpoTestRuntime expo;
  expo.eval("globalThis.K = expo.modules.ExpoKeepAwake");

  CHECK(expo.settle("K.isAvailableAsync()") == "true");
  CHECK(expo.settle("K.isActivated()") == "false");
  CHECK(expo.settle("K.activate('video')") == "true");
  CHECK(expo.settle("K.activate('download')") == "true");
  CHECK_FALSE(SDL_ScreenSaverEnabled());
  CHECK(expo.settle("K.deactivate('video')") == "true");
  CHECK(expo.settle("K.isActivated()") == "true");
  CHECK_FALSE(SDL_ScreenSaverEnabled());
  CHECK(expo.settle("K.deactivate('download')") == "true");
  CHECK(expo.settle("K.deactivate('never-activated')") == "true");
  CHECK(expo.settle("K.isActivated()") == "false");
  CHECK(SDL_ScreenSaverEnabled());
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

TEST_CASE("keep awake is unavailable and changes nothing without a window system") {
  REQUIRE(SDL_WasInit(SDL_INIT_VIDEO) == 0);
  testing::ExpoTestRuntime expo;
  expo.eval("globalThis.K = expo.modules.ExpoKeepAwake");

  CHECK(expo.settle("K.isAvailableAsync()") == "false");
  CHECK(expo.settle("K.activate('video')") == "false");
  CHECK(expo.settle("K.isActivated()") == "true");
  CHECK(expo.settle("K.deactivate('video')") == "false");
}

} // namespace rnwebos::expo
