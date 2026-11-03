#include "ViewSmartObject.h"

namespace rntizen {

namespace {

void moveAlone(Evas_Object *, Evas_Coord, Evas_Coord) {}

Evas_Smart *viewSmart() {
  static Evas_Smart *smart = [] {
    static Evas_Smart_Class smartClass = EVAS_SMART_CLASS_INIT_NAME_VERSION("RNTizenView");
    evas_object_smart_clipped_smart_set(&smartClass);
    smartClass.move = moveAlone;
    return evas_smart_class_new(&smartClass);
  }();
  return smart;
}

} // namespace

Evas_Object *addViewSmartObject(Evas *evas) {
  return evas_object_smart_add(evas, viewSmart());
}

} // namespace rntizen
