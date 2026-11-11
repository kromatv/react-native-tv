#pragma once

#include "../views/ComponentView.h"
#include "SvgRootNode.h"

namespace rnwebos::svg {

/// `<Svg>`: draws the tree of its nodes straight onto the canvas, clipped to
/// the view's box. User units are layout points, scaled to pixels after the
/// viewBox transform.
class SvgView : public ComponentView {
 public:
  SvgView(Scene &scene, facebook::react::Tag tag);

  void updateProps(const facebook::react::Props::Shared &props) override;

 protected:
  void drawContent(SkCanvas &canvas, float alpha) override;
  void drawChildren(SkCanvas &canvas) override;
  int contentLayers() const override;
  void didChangeChildren() override;
  virtual void readProps(const facebook::react::Props &props) = 0;

  template <typename Props>
  void readSvgProps(const Props &props) {
    root_.viewBox = {
        .minX = props.minX,
        .minY = props.minY,
        .width = props.vbWidth,
        .height = props.vbHeight,
        .align = props.align,
        .meetOrSlice = props.meetOrSlice,
    };
    root_.color = colorFromProp(props.color);
  }

 private:
  SkSize sizeInPoints() const;

  RootNode root_;
};

} // namespace rnwebos::svg
