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

SkSize SvgView::sizeInPoints() const {
  const auto &size = layoutMetrics().frame.size;
  return {static_cast<float>(size.width), static_cast<float>(size.height)};
}

void SvgView::drawContent(SkCanvas &canvas, float alpha) {
  root_.renderInto(canvas, sizeInPoints(), static_cast<float>(pointScale()), alpha);
}

void SvgView::drawChildren(SkCanvas &) {
  // The children are the nodes `drawContent` draws, not views with a box each.
}

int SvgView::contentLayers() const {
  return root_.paintLayers(sizeInPoints());
}

void SvgView::didChangeChildren() {
  root_.setChildren(nodesOf(children()));
}

RN_WEBOS_COMPONENT_VIEW("RNSVGSvgView", SvgRootView<facebook::react::RNSVGSvgViewProps>);
RN_WEBOS_COMPONENT_VIEW("RNSVGSvgViewAndroid", SvgRootView<facebook::react::RNSVGSvgViewAndroidProps>);

} // namespace rnwebos::svg
