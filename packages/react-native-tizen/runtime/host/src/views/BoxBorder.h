#pragma once

#include "BoxStyle.h"
#include "PixelGeometry.h"

#include <Evas.h>

#include <array>
#include <vector>

namespace rntizen {

/// A view's border and outline, drawn above its children. One color: a tinted
/// ring nine-patch, which follows rounded corners; dashed or dotted, a ring
/// painted at the box's size. A color per side: four solid rectangles,
/// square-cornered; with rounded corners the ring takes the top color.
class BoxBorder final {
 public:
  BoxBorder(Evas *evas, Evas_Object *smart);
  ~BoxBorder();

  BoxBorder(const BoxBorder &) = delete;
  BoxBorder &operator=(const BoxBorder &) = delete;

  /// Returns true when objects were added or removed.
  bool update(const BoxStyle &style);
  void layout(const PixelRect &box);
  std::vector<Evas_Object *> objects() const;
  /// How far the outline reaches past the box on each side.
  int overflow() const;

 private:
  bool updateRing(const BoxStyle &style, bool wanted);
  bool updateSides(const BoxStyle &style, bool wanted);
  bool updateOutline(const BoxStyle &style);
  void paintPattern(const PixelRect &box);

  Evas *const evas_;
  Evas_Object *const smart_;
  Evas_Object *ring_{nullptr};
  std::array<Evas_Object *, 4> sides_{};
  Evas_Object *outline_{nullptr};
  PixelRect patterned_;
  BoxStyle style_;
};

} // namespace rntizen
