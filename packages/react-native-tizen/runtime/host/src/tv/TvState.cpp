#include "TvState.h"

namespace rntizen {

bool TvState::listening(std::string_view event) const {
  return event == "search" ? searchListeners_.load() > 0 : commandListeners_.load() > 0;
}

void TvState::addListener(std::string_view event, int delta) {
  (event == "search" ? searchListeners_ : commandListeners_) += delta;
}

} // namespace rntizen
