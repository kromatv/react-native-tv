#pragma once

#include "NinePatch.h"

#include <Evas.h>

#include <cstddef>
#include <vector>

namespace rntizen {

/// A shown rectangle member of `smart`, transparent to input.
Evas_Object *addRectangleMember(Evas *evas, Evas_Object *smart);

/// A shown, filled image member of `smart`, transparent to input, ready for a nine-patch.
Evas_Object *addPatchImage(Evas *evas, Evas_Object *smart);

/// Adds patch images to the end of `images`, or deletes them from it, until it
/// holds `count`. Returns true when the count changed.
bool resizePatchImages(std::vector<Evas_Object *> &images, std::size_t count, Evas *evas, Evas_Object *smart);

/// Uploads `patch` into `image` and sets its border.
void setNinePatch(Evas_Object *image, const NinePatch &patch);

} // namespace rntizen
