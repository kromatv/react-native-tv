#pragma once

#include "ComponentView.h"
#include "ScrollAnimation.h"

#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>

#include <memory>

namespace rnwebos {

/// A `<ScrollView>`, vertical or horizontal: clips its content and shifts it
/// by the content offset. The offset moves with `scrollTo` and `scrollToEnd`
/// (a TV has no touch, so there is no drag or fling) and with `scrollBy`, is
/// written back into the shadow tree's state so `measure` sees it, and is
/// reported through `onScroll`, throttled by `scrollEventThrottle`, and
/// `onMomentumScrollEnd`.
class ScrollView final : public ComponentView {
 public:
  using ComponentView::ComponentView;
  ~ScrollView() override;

  void updateProps(const facebook::react::Props::Shared &props) override;
  void updateState(const facebook::react::State::Shared &state) override;
  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;
  void handleCommand(const std::string &name, const folly::dynamic &args) override;

  /// Moves the content by `dx` and `dy` points at once, as a wheel does,
  /// stopping at the content's edges. Returns false when it could not move.
  bool scrollBy(facebook::react::Float dx, facebook::react::Float dy);

  facebook::react::Point contentOffset() const;

 protected:
  facebook::react::Point childrenOffset() const override;

 private:
  void scrollTo(facebook::react::Point offset, bool animated);
  void moveContent(facebook::react::Point offset);
  void emitScroll(bool throttled);
  void finishScrolling();
  void settleSoon();
  void cancelSettle();
  void keepOffsetInContent();
  facebook::react::Point clamped(facebook::react::Point offset) const;
  facebook::react::Size contentSize() const;
  facebook::react::ScrollEvent scrollEvent() const;

  std::shared_ptr<const facebook::react::ScrollViewShadowNode::ConcreteState> state_;
  facebook::react::Point contentOffset_{};
  facebook::react::Point propsContentOffset_{};
  ScrollAnimation animation_;
  MainLoop::Id settleTimer_{0};
  double scrollEventThrottle_{0};
  double lastScrollEvent_{0};
};

} // namespace rnwebos
