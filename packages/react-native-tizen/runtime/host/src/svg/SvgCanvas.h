#pragma once

#include "SvgNode.h"

#include <Evas.h>

#include <optional>
#include <vector>

namespace rntizen::svg {

/// One Evas VG object holding an `<Svg>`'s shapes, in coordinates relative to
/// the object's top-left corner. Shapes and gradients are reused from one paint
/// to the next, so painting again allocates nothing once their counts have been
/// reached. Gradients live in a container that is never the drawn root: Evas
/// draws and bounds every node under the root (efl-rendering.md, section 8).
/// Main thread only.
class SvgCanvas {
 public:
  explicit SvgCanvas(Evas *evas);
  ~SvgCanvas();

  SvgCanvas(const SvgCanvas &) = delete;
  SvgCanvas &operator=(const SvgCanvas &) = delete;

  Evas_Object *object() const;
  void paint(const Drawing &drawing, double width, double height);

 private:
  struct GradientCursor {
    std::size_t linear{0};
    std::size_t radial{0};
  };

  Evas_Vg_Shape *shapeAt(std::size_t index);
  void paintShape(Evas_Vg_Shape *shape, const DrawnShape &drawn, GradientCursor &cursor);
  Evas_Vg_Gradient *gradientFor(const std::optional<GradientPaint> &paint, GradientCursor &cursor);

  Evas_Object *const object_;
  Evas_Vg_Container *const root_;
  Evas_Vg_Container *const gradients_;
  Evas_Vg_Shape *const extent_;
  std::vector<Evas_Vg_Shape *> shapes_;
  std::vector<Evas_Vg_Gradient_Linear *> linears_;
  std::vector<Evas_Vg_Gradient_Radial *> radials_;
};

} // namespace rntizen::svg
