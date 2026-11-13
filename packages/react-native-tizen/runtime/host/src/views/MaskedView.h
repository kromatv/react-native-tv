#pragma once

#include "ComponentView.h"

namespace rntizen {

/// `@react-native-masked-view/masked-view`'s `RNCMaskedView`: the first child
/// is the mask, and the children after it show only where its alpha lets them,
/// inside the view's own box. With the mask alone nothing shows.
class MaskedView final : public ComponentView {
 public:
  MaskedView(Evas *evas, facebook::react::Tag tag);
  ~MaskedView() override;

 private:
  void layoutMembers() override;
  Evas_Object *clipperFor(const ComponentView &child) const override;
  Evas_Object *hostFor(const ComponentView &child) const override;
  void didChangeChildren() override;
  bool isMask(const ComponentView &child) const;
  void refreshMask();

  Evas_Object *const stage_;
  Evas_Object *const tick_;
  Evas_Object *const mask_;
  PixelRect shown_;
  bool ticked_{false};
};

} // namespace rntizen
