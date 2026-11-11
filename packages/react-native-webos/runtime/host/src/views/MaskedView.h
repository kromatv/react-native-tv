#pragma once

#include "ComponentView.h"

namespace rnwebos {

/// `@react-native-masked-view/masked-view`'s `RNCMaskedView`: the first child
/// is the mask, and the children after it show only where the mask's alpha
/// lets them. With the mask alone nothing shows.
class MaskedView final : public ComponentView {
 public:
  using ComponentView::ComponentView;

 protected:
  void drawChildren(SkCanvas &canvas) override;
};

} // namespace rnwebos
