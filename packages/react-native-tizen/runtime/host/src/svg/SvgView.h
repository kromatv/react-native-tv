#pragma once

#include "../views/ComponentView.h"
#include "SvgCanvas.h"
#include "SvgNodeView.h"
#include "ViewBox.h"

#include <Ecore.h>

#include <memory>

namespace rntizen::svg {

/// `<Svg>`: one Evas VG object, clipped to the view, that its nodes paint into
/// in canvas pixels. Any change in the subtree, and a new size or scale,
/// schedules a single repaint on the main loop, after the Fabric transaction
/// that caused it; a move repaints nothing.
class SvgView : public ComponentView {
 public:
  SvgView(Evas *evas, facebook::react::Tag tag);
  ~SvgView() override;

  void updateProps(const facebook::react::Props::Shared &props) override;

  std::shared_ptr<const RedrawRequest> redrawRequest() const;

 protected:
  void layoutMembers() override;
  virtual void readProps(const facebook::react::Props &props) = 0;

  template <typename Props>
  void readSvgProps(const Props &props) {
    viewBox_ = {
        .minX = props.minX,
        .minY = props.minY,
        .width = props.vbWidth,
        .height = props.vbHeight,
        .align = props.align,
        .meetOrSlice = props.meetOrSlice,
    };
    color_ = rgbaFromColor(props.color).value_or(Rgba{});
  }

 private:
  static void onRedrawJob(void *data);
  void scheduleRedraw();
  void redraw();

  SvgCanvas canvas_;
  ViewBox viewBox_;
  Rgba color_;
  Ecore_Job *job_{nullptr};
  PixelRect painted_;
  std::shared_ptr<const RedrawRequest> redrawRequest_;
};

} // namespace rntizen::svg
