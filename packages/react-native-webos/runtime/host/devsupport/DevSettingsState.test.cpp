#include "DevSettingsState.h"

#include <doctest/doctest.h>

namespace rntizen {

TEST_CASE("Fast Refresh starts on, and only a change asks for a reload") {
  DevSettingsState state;

  CHECK(state.hotLoadingEnabled());
  CHECK_FALSE(state.setHotLoadingEnabled(true));
  CHECK(state.setHotLoadingEnabled(false));
  CHECK_FALSE(state.hotLoadingEnabled());
  CHECK_FALSE(state.setHotLoadingEnabled(false));
}

TEST_CASE("LogBox's surface starts and stops once per change") {
  DevSettingsState state;

  CHECK_FALSE(state.setLogBoxShown(false));
  CHECK(state.setLogBoxShown(true));
  CHECK_FALSE(state.setLogBoxShown(true));
  CHECK(state.setLogBoxShown(false));
}

TEST_CASE("A new instance has no LogBox surface, and keeps Fast Refresh as it was") {
  DevSettingsState state;
  state.setHotLoadingEnabled(false);
  state.setLogBoxShown(true);

  state.instanceStarted();

  CHECK(state.setLogBoxShown(true));
  CHECK_FALSE(state.hotLoadingEnabled());
}

} // namespace rntizen
