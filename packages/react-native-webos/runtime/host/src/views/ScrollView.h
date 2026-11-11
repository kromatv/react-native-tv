#pragma once

#include "ComponentView.h"
#include "ScrollAnimation.h"

#include <react/renderer/components/scrollview/ScrollViewShadowNode.h>

#include <memory>

namespace rntizen {

/// A `<ScrollView>`, vertical or horizontal: clips its content and shifts it
/// by the content offset. The offset moves with `scrollTo` and `scrollToEnd`
/// (a TV has no touch, so there is no drag or fling), is written back into
/// the shadow tree's state so `measure` sees it, and is reported through
/// `onScroll`, throttled by `scrollEventThrottle`, and `onMomentumScrollEnd`.
class ScrollView final : public ComponentView {
 public:
  ScrollView(Evas *evas, facebook::react::Tag tag);

  void updateProps(const facebook::react::Props::Shared &props) override;
  void updateState(const facebook::react::State::Shared &state) override;
  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;
  void handleCommand(const std::string &name, const folly::dynamic &args) override;

 protected:
  facebook::react::Point childrenOffset() const override;

 private:
  void scrollTo(facebook::react::Point offset, bool animated);
  void moveContent(facebook::react::Point offset);
  void emitScroll(bool throttled);
  void finishScrolling();
  void keepOffsetInContent();
  facebook::react::Point clamped(facebook::react::Point offset) const;
  facebook::react::Size contentSize() const;
  facebook::react::ScrollEvent scrollEvent() const;

  std::shared_ptr<const facebook::react::ScrollViewShadowNode::ConcreteState> state_;
  facebook::react::Point contentOffset_{};
  facebook::react::Point propsContentOffset_{};
  ScrollAnimation animation_;
  double scrollEventThrottle_{0};
  double lastScrollEvent_{0};
};

} // namespace rntizen
