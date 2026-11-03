#pragma once

#include "NinePatch.h"

#include <Evas.h>

namespace rntizen {

/// A member of a view's smart object that fills its box: a rectangle while its
/// corners are square, a filled nine-patch image once they are rounded.
class BoxShape final {
 public:
  BoxShape(Evas *evas, Evas_Object *smart);
  ~BoxShape();

  BoxShape(const BoxShape &) = delete;
  BoxShape &operator=(const BoxShape &) = delete;

  /// Shows the shape with `corners`. Returns true when its object was added or
  /// replaced.
  bool show(const Corners &corners);
  /// Deletes the object. Returns true when there was one.
  bool remove();
  /// The object, or null when there is none.
  Evas_Object *object() const;

 private:
  Evas *const evas_;
  Evas_Object *const smart_;
  Evas_Object *object_{nullptr};
  Corners corners_;
};

} // namespace rntizen
