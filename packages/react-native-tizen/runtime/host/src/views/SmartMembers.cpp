#include "SmartMembers.h"

namespace rntizen {

Evas_Object *addRectangleMember(Evas *evas, Evas_Object *smart) {
  auto *rectangle = evas_object_rectangle_add(evas);
  evas_object_pass_events_set(rectangle, EINA_TRUE);
  evas_object_smart_member_add(rectangle, smart);
  evas_object_show(rectangle);
  return rectangle;
}

Evas_Object *addPatchImage(Evas *evas, Evas_Object *smart) {
  auto *image = evas_object_image_filled_add(evas);
  evas_object_pass_events_set(image, EINA_TRUE);
  evas_object_image_alpha_set(image, EINA_TRUE);
  evas_object_smart_member_add(image, smart);
  evas_object_show(image);
  return image;
}

bool resizePatchImages(std::vector<Evas_Object *> &images, std::size_t count, Evas *evas, Evas_Object *smart) {
  if (images.size() == count) return false;
  while (images.size() > count) {
    evas_object_del(images.back());
    images.pop_back();
  }
  while (images.size() < count) images.push_back(addPatchImage(evas, smart));
  return true;
}

void setNinePatch(Evas_Object *image, const NinePatch &patch) {
  evas_object_image_size_set(image, patch.width, patch.height);
  evas_object_image_data_copy_set(image, const_cast<std::uint32_t *>(patch.pixels.data()));
  evas_object_image_data_update_add(image, 0, 0, patch.width, patch.height);
  evas_object_image_border_set(image, patch.border.left, patch.border.right, patch.border.top, patch.border.bottom);
  evas_object_image_border_center_fill_set(image, patch.hollow ? EVAS_BORDER_FILL_NONE : EVAS_BORDER_FILL_DEFAULT);
}

} // namespace rntizen
