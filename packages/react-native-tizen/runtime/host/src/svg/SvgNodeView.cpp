#include "SvgNodeView.h"

#include "../mounting/ComponentViewRegistry.h"
#include "SvgView.h"

#include <react/renderer/components/rnsvg/Props.h>

namespace rntizen::svg {

namespace {

std::shared_ptr<const RedrawRequest> ownerRedraw(const ComponentView &node) {
  for (auto *ancestor = node.parent(); ancestor != nullptr; ancestor = ancestor->parent()) {
    if (const auto *svg = dynamic_cast<const SvgView *>(ancestor)) return svg->redrawRequest();
  }
  return nullptr;
}

class SvgGroupView final : public SvgNodeView {
 public:
  using SvgNodeView::SvgNodeView;

 protected:
  void readProps(const facebook::react::Props &props) override {
    readRenderable(static_cast<const facebook::react::RNSVGGroupProps &>(props));
  }

  void drawContent(Drawing &drawing, const DrawContext &context) const override {
    drawChildren(children(), drawing, context);
  }
};

} // namespace

SvgNodeView::SvgNodeView(Evas *evas, facebook::react::Tag tag) : ComponentView(evas, tag) {}

SvgNodeView::~SvgNodeView() {
  requestRedraw();
}

void SvgNodeView::updateProps(const facebook::react::Props::Shared &props) {
  ComponentView::updateProps(props);
  readProps(*props);
  requestRedraw();
}

void SvgNodeView::draw(Drawing &drawing, const DrawContext &parent) const {
  if (node_.hidden) return;
  DrawContext context = parent;
  context.transform = parent.transform * node_.matrix;
  context.opacity = parent.opacity * node_.opacity;
  context.paint = inheritPaint(paint_, parent.paint);
  if (node_.color) context.currentColor = *node_.color;
  drawContent(drawing, context);
}

void SvgNodeView::didMount() {
  requestRedraw();
}

void SvgNodeView::willUnmount() {
  requestRedraw();
}

void SvgNodeView::requestRedraw() {
  if (auto owner = ownerRedraw(*this)) redraw_ = owner;
  if (const auto redraw = redraw_.lock()) (*redraw)();
}

void drawChildren(const std::vector<ComponentView *> &children, Drawing &drawing, const DrawContext &context) {
  for (const auto *child : children) {
    if (const auto *node = dynamic_cast<const SvgNodeView *>(child)) node->draw(drawing, context);
  }
}

RN_TIZEN_COMPONENT_VIEW("RNSVGGroup", SvgGroupView);

} // namespace rntizen::svg
