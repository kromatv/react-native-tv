#include "VideoWindow.h"

#include <map>

namespace rnwebos::expo {

void videoViewShown(Scene &scene, bool shown) {
  static std::map<Scene *, int> shownViews;
  auto &count = shownViews[&scene];
  count += shown ? 1 : -1;
  scene.setTransparent(count > 0);
  if (count <= 0) shownViews.erase(&scene);
}

} // namespace rnwebos::expo
