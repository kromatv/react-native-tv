#include "BoxShape.h"

#include "SmartMembers.h"

namespace rntizen {

namespace {

constexpr int kWholeImagePixels = 512 * 512;

} // namespace

BoxShape::BoxShape(Evas *evas, Evas_Object *smart, bool clips) : evas_(evas), smart_(smart), clips_(clips) {}

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
  if (rounded && (replaced || corners != corners_)) {
    setNinePatch(object_, filledPatch(corners));
    whole_ = false;
  }
  corners_ = corners;
  return replaced;
}

void BoxShape::layout(const PixelRect &box) {
  if (object_ == nullptr) return;
  setGeometry(object_, box);
  if (!clips_ || corners_.isSquare()) return;
  const bool whole = box.width > 0 && box.height > 0 && box.width * box.height <= kWholeImagePixels;
  if (whole == whole_ && (!whole || (box.width == width_ && box.height == height_))) return;
  width_ = box.width;
  height_ = box.height;
  setNinePatch(object_, whole ? wholeFilledPatch(corners_, width_, height_) : filledPatch(corners_));
  whole_ = whole;
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
