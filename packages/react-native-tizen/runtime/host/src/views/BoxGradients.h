#pragma once

#include "BoxStyle.h"
#include "PixelGeometry.h"

#include <Evas.h>

#include <vector>

namespace rntizen {

/// A view's `backgroundImage`: one image per gradient layer over the padding
/// box, cut to its inner corners, above the background color and below the
/// content, the first layer on top as CSS stacks them. An image is painted
/// again only when its layer, the corners or the box's size change.
class BoxGradients final {
 public:
  BoxGradients(Evas *evas, Evas_Object *smart);
  ~BoxGradients();

  BoxGradients(const BoxGradients &) = delete;
  BoxGradients &operator=(const BoxGradients &) = delete;

  /// Returns true when images were added or removed.
  bool update(const BoxStyle &style);
  void layout(const PixelRect &box);
  /// The images, lowest first.
  const std::vector<Evas_Object *> &objects() const;

 private:
  void paint(const PixelRect &area);

  Evas *const evas_;
  Evas_Object *const smart_;
  std::vector<Evas_Object *> images_;
  BoxStyle style_;
  PixelRect painted_;
  bool stale_{true};
};

} // namespace rntizen
