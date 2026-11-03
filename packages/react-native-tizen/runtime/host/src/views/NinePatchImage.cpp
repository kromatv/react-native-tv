#include "NinePatchImage.h"

namespace rntizen {

Evas_Object *addPatchImage(Evas *evas, Evas_Object *smart) {
  auto *image = evas_object_image_filled_add(evas);
  evas_object_pass_events_set(image, EINA_TRUE);
  evas_object_image_alpha_set(image, EINA_TRUE);
  evas_object_smart_member_add(image, smart);
  evas_object_show(image);
  return image;
}

void setNinePatch(Evas_Object *image, const NinePatch &patch) {
  evas_object_image_size_set(image, patch.width, patch.height);
  evas_object_image_data_copy_set(image, const_cast<std::uint32_t *>(patch.pixels.data()));
  evas_object_image_data_update_add(image, 0, 0, patch.width, patch.height);
  evas_object_image_border_set(image, patch.border.left, patch.border.right, patch.border.top, patch.border.bottom);
}

} // namespace rntizen
