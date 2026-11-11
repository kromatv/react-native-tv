#pragma once

#include "../views/ComponentView.h"
#include "SvgRootNode.h"

#include <utility>

namespace rnwebos::svg {

/// `<Svg>`: draws the tree of its nodes onto the canvas, clipped to the
/// view's box. User units are layout points, scaled to pixels after the
/// viewBox transform.
class SvgView : public ComponentView {
 public:
  SvgView(Scene &scene, facebook::react::Tag tag);

  void updateProps(const facebook::react::Props::Shared &props) override;

 protected:
  void drawContent(SkCanvas &canvas, float alpha) override;
  void drawChildren(SkCanvas &canvas) override;
  void didChangeChildren() override;
  virtual void readProps(const facebook::react::Props &props) = 0;

  /// Takes the props the drawing depends on. The others, such as the view's
  /// own opacity or transform while they animate, leave the drawing as it is.
  template <typename Props>
  void readSvgProps(const Props &props) {
    SvgViewBox viewBox{
        .minX = props.minX,
        .minY = props.minY,
        .width = props.vbWidth,
        .height = props.vbHeight,
        .align = props.align,
        .meetOrSlice = props.meetOrSlice,
    };
    const auto color = colorFromProp(props.color);
    if (viewBox == root_.viewBox && color == root_.color) return;
    root_.viewBox = std::move(viewBox);
    root_.color = color;
    root_.invalidate();
  }

 private:
  RootNode root_;
};

} // namespace rnwebos::svg
