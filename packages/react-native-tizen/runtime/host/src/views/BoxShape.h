#pragma once

#include "NinePatch.h"
#include "PixelGeometry.h"

#include <Evas.h>

namespace rntizen {

/// A member of a view's smart object that fills its box: a rectangle while its
/// corners are square, a filled nine-patch image once they are rounded. A
/// rounded shape that clips is one image the size of its box while the box is
/// small: the GL engine takes such a mask as it is, where a nine-patch gets a
/// surface of its own and is drawn into it again each time it moves
/// (efl-rendering.md section 3).
class BoxShape final {
 public:
  BoxShape(Evas *evas, Evas_Object *smart, bool clips = false);
  ~BoxShape();

  BoxShape(const BoxShape &) = delete;
  BoxShape &operator=(const BoxShape &) = delete;

  /// Shows the shape with `corners`. Returns true when its object was added or
  /// replaced.
  bool show(const Corners &corners);
  /// Places the shape over `box`.
  void layout(const PixelRect &box);
  /// Deletes the object. Returns true when there was one.
  bool remove();
  /// The object, or null when there is none.
  Evas_Object *object() const;

 private:
  Evas *const evas_;
  Evas_Object *const smart_;
  const bool clips_;
  Evas_Object *object_{nullptr};
  Corners corners_;
  int width_{0};
  int height_{0};
  bool whole_{false};
};

} // namespace rntizen
