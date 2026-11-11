#pragma once

#include "ComponentView.h"

namespace rntizen {

/// A `<Modal>`: drawn above every other view at the screen's origin, across
/// the logical screen, whatever its place in the tree. Emits `onShow` once
/// laid out. While mounted it takes Back from `BackHandler`, as a Modal's
/// dialog does on Android: the Modal mounted last gets `onRequestClose`.
class ModalHostView final : public ComponentView {
 public:
  using ComponentView::ComponentView;
  ~ModalHostView() override;

  void updateLayout(const facebook::react::LayoutMetrics &layoutMetrics) override;

  /// Sends `onRequestClose` to the Modal mounted last on `evas`. Returns
  /// false when no Modal is mounted there, so Back goes to `BackHandler`.
  static bool requestCloseTopmost(Evas *evas);

 protected:
  bool isOverlay() const override;
  void didMount() override;
  void willUnmount() override;

 private:
  void leaveStack();

  bool shown_{false};
};

} // namespace rntizen
