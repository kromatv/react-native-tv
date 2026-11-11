#include "LogicalScreen.h"

#include <algorithm>

namespace rnwebos {

facebook::react::Float pointScaleFactor(facebook::react::Size windowSize) {
  if (windowSize.width <= 0 || windowSize.height <= 0) return 1;
  return std::min(windowSize.width / kLogicalScreenSize.width, windowSize.height / kLogicalScreenSize.height);
}

} // namespace rnwebos
