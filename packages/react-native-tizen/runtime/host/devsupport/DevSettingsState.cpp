#include "DevSettingsState.h"

#include <utility>

namespace rntizen {

bool DevSettingsState::hotLoadingEnabled() const {
  return hotLoadingEnabled_;
}

bool DevSettingsState::setHotLoadingEnabled(bool enabled) {
  return std::exchange(hotLoadingEnabled_, enabled) != enabled;
}

bool DevSettingsState::setLogBoxShown(bool shown) {
  return std::exchange(logBoxShown_, shown) != shown;
}

void DevSettingsState::instanceStarted() {
  logBoxShown_ = false;
}

} // namespace rntizen
