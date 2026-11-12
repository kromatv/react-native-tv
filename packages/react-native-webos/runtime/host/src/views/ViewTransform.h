#pragma once

#include <react/renderer/components/view/ViewProps.h>
#include <react/renderer/core/LayoutMetrics.h>
#include <react/renderer/graphics/Transform.h>

#include "include/core/SkCanvas.h"
#include "include/core/SkM44.h"

namespace rnwebos {

/// A view's `transform` around its `transformOrigin` (the center by default,
/// as on iOS and Android), as the matrix Skia draws the view's subtree through.
class ViewTransform final {
 public:
  /// Returns true when the transform changed.
  bool update(const facebook::react::ViewProps &props, const facebook::react::LayoutMetrics &metrics);

  bool isIdentity() const;

  /// Transforms `canvas`, whose origin is the view's top-left corner, in
  /// pixels at `scale` pixels per point. A plain translation moves by whole
  /// pixels, so what it carries stays sharp.
  void concat(SkCanvas &canvas, facebook::react::Float scale) const;

  /// Where the point `local`, in the view's own pixels, shows in the pixels
  /// the view is placed in.
  SkPoint mapPoint(SkPoint local, facebook::react::Float scale) const;

  /// The rectangle around where `local`, in the view's own pixels, shows in
  /// the pixels the view is placed in. A transform with perspective gives a
  /// rectangle that holds everything, since part of the view may be behind
  /// the viewer.
  SkRect mapRect(const SkRect &local, facebook::react::Float scale) const;

  /// The point of the view's own pixels that shows at `outer`, or nothing
  /// when the transform flattens the view.
  bool invertPoint(SkPoint outer, facebook::react::Float scale, SkPoint &local) const;

 private:
  SkM44 pixelMatrix(facebook::react::Float scale) const;

  facebook::react::Transform matrix_;
  bool isIdentity_{true};
  bool isTranslation_{true};
};

} // namespace rnwebos
