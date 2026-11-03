#include "VideoWindow.h"

#include "HostWindow.h"

namespace rntizen::expo {

namespace {

struct WindowState {
  int shownViews{0};
  bool transparent{false};
};

WindowState &windowState() {
  static WindowState state;
  return state;
}

} // namespace

void videoViewShown(bool shown) {
  auto &state = windowState();
  state.shownViews += shown ? 1 : -1;
  const bool transparent = state.shownViews > 0;
  if (transparent == state.transparent) return;
  state.transparent = transparent;
  setHostWindowTransparent(transparent);
}

} // namespace rntizen::expo
