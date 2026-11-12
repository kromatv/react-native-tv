#include "BoxShape.h"

#include "SmartMembers.h"

namespace rntizen {

BoxShape::BoxShape(Evas *evas, Evas_Object *smart) : evas_(evas), smart_(smart) {}

BoxShape::~BoxShape() {
  remove();
}

bool BoxShape::show(const Corners &corners) {
  const bool rounded = !corners.isSquare();
  const bool replaced = object_ == nullptr || corners_.isSquare() == rounded;
  if (replaced) {
    remove();
    object_ = rounded ? addPatchImage(evas_, smart_) : addRectangleMember(evas_, smart_);
  }
  if (rounded && (replaced || corners != corners_)) setNinePatch(object_, filledPatch(corners));
  corners_ = corners;
  return replaced;
}

bool BoxShape::remove() {
  if (object_ == nullptr) return false;
  evas_object_del(object_);
  object_ = nullptr;
  return true;
}

Evas_Object *BoxShape::object() const {
  return object_;
}

} // namespace rntizen
