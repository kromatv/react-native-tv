#pragma once

#include "BoxBorder.h"
#include "BoxGradients.h"
#include "BoxShadows.h"
#include "BoxShape.h"
#include "BoxStyle.h"
#include "PixelGeometry.h"

#include <Evas.h>

#include <vector>

namespace rntizen {

/// What a view draws of its own box, as members of its smart object: shadows,
/// background color and gradients, border, outline, and the clipper its
/// content and children hang from when it clips. Rounded corners are nine-patch images; a rounded clip is
/// an image clipper, which Evas renders as a mask (efl-rendering.md section 4).
class ViewBox final {
 public:
  ViewBox(Evas *evas, Evas_Object *smart);

  ViewBox(const ViewBox &) = delete;
  ViewBox &operator=(const ViewBox &) = delete;

  /// Returns true when objects were added or removed, so the view restacks
  /// its members and hangs its content from `clipper()` again.
  bool update(const BoxStyle &style);
  void layout(const PixelRect &box);

  /// What content and children are clipped to, or null when the box does not clip.
  Evas_Object *clipper() const;
  /// Shadows, the background color, then the gradients, lowest first. A box
  /// that neither paints a color nor clips has no background object.
  std::vector<Evas_Object *> objectsBelowContent() const;
  /// The border and the outline, drawn above the children.
  std::vector<Evas_Object *> objectsAboveChildren() const;
  /// How far shadows and the outline draw past the box on each side.
  Edges overflow() const;

 private:
  bool updateBackground(const BoxStyle &style);

  Evas_Object *const smart_;
  BoxShadows shadows_;
  BoxBorder border_;
  BoxGradients gradients_;
  BoxShape background_;
  BoxShape clipper_;
  bool updated_{false};
  BoxStyle style_;
};

} // namespace rntizen
