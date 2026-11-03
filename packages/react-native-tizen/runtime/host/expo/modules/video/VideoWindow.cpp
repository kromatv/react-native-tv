#include "VideoWindow.h"

#include "HostWindow.h"

namespace rntizen::expo {

namespace {

int shownViews = 0;
bool windowTransparent = false;

} // namespace

void videoViewShown(bool shown) {
  shownViews += shown ? 1 : -1;
  const bool transparent = shownViews > 0;
  if (transparent == windowTransparent) return;
  windowTransparent = transparent;
  setHostWindowTransparent(transparent);
}

} // namespace rntizen::expo
