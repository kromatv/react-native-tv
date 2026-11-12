#pragma once

#include "../views/ComponentView.h"

#include <folly/dynamic.h>
#include <react/renderer/graphics/Rect.h>

#include <string>
#include <vector>

namespace rntizen {

/// A view on screen that voice can name: its `accessibilityLabel` and where it
/// is, in points of the logical screen.
struct LabelledView {
  ComponentView *view;
  std::string label;
  facebook::react::Rect frame;
};

/// Every visible view under `root` with an `accessibilityLabel`, in reading
/// order, top to bottom then left to right. A labelled view's own children are
/// not listed. Main thread only.
std::vector<LabelledView> labelledViews(ComponentView &root);

/// The views as Samsung's content context: `[{positionX, positionY, title,
/// alias, bFocused}]`, positions at each view's center.
folly::dynamic contentContext(const std::vector<LabelledView> &views);

/// The view whose label is `title`, ignoring case, spaces and punctuation;
/// else the first whose label starts with it, else contains it; null for none.
const LabelledView *findByTitle(const std::vector<LabelledView> &views, const std::string &title);

/// Presses `view` as a tap at its center would: a touch start and end on it, so
/// a Pressable or Touchable runs its `onPress`. Returns false when the view has
/// no touch events to send.
bool tapView(const LabelledView &view);

} // namespace rntizen
