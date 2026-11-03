#pragma once

#include "PixelGeometry.h"

#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/graphics/Transform.h>

#include <Evas.h>

namespace rntizen {

/// A view's `transform` around its `transformOrigin` (the center by default,
/// as on iOS and Android). A plain translation moves the view; anything else
/// maps its smart object with four points (efl-rendering.md section 3), which
/// renders the subtree into a surface the size of the object's geometry.
class ViewTransform final {
 public:
  /// Returns true when the transform changed.
  bool update(const facebook::react::ViewProps &props, const facebook::react::LayoutMetrics &metrics);

  /// The offset a translation-only transform moves the view by, else zero.
  PixelPoint translation(facebook::react::Float scale) const;

  /// Maps `root` so that its `bounds` show transformed around `box`, the
  /// view's untransformed box; removes any map when none is needed.
  void apply(Evas_Object *root, const PixelRect &box, const PixelRect &bounds, facebook::react::Float scale) const;

  /// Where `point`, in window pixels, shows once `apply` mapped the view whose
  /// untransformed box is `box`. A translation is already in `box`, so it
  /// leaves the point where it is.
  PixelPoint mapPoint(PixelPoint point, const PixelRect &box, facebook::react::Float scale) const;

 private:
  facebook::react::Transform matrix_;
  bool isTranslation_{true};
};

} // namespace rntizen
