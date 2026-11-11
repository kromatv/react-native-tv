#include "SvgView.h"

#include "../mounting/ComponentViewRegistry.h"
#include "SvgNodeView.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rnwebos::svg {

namespace {

template <typename Props>
class SvgRootView final : public SvgView {
 public:
  using SvgView::SvgView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    readSvgProps(static_cast<const Props &>(props));
  }
};

} // namespace

SvgView::SvgView(Scene &scene, facebook::react::Tag tag) : ComponentView(scene, tag) {
  setAlwaysClips(true);
  root_.onChange = [this] { invalidate(); };
}

void SvgView::updateProps(const facebook::react::Props::Shared &props) {
  ComponentView::updateProps(props);
  readProps(*props);
  root_.invalidate();
}

void SvgView::drawContent(SkCanvas &canvas, float) {
  const auto &size = layoutMetrics().frame.size;
  root_.renderInto(
      canvas, {static_cast<float>(size.width), static_cast<float>(size.height)}, static_cast<float>(pointScale()));
}

void SvgView::drawChildren(SkCanvas &) {
  // The children are the nodes `drawContent` draws, not views with a box each.
}

void SvgView::didChangeChildren() {
  root_.setChildren(nodesOf(children()));
}

RN_WEBOS_COMPONENT_VIEW("RNSVGSvgView", SvgRootView<facebook::react::RNSVGSvgViewProps>);
RN_WEBOS_COMPONENT_VIEW("RNSVGSvgViewAndroid", SvgRootView<facebook::react::RNSVGSvgViewAndroidProps>);

} // namespace rnwebos::svg
