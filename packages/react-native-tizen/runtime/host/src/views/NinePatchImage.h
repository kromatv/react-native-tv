#pragma once

#include "NinePatch.h"

#include <Evas.h>

namespace rntizen {

/// A shown, filled image member of `smart`, transparent to input, ready for a nine-patch.
Evas_Object *addPatchImage(Evas *evas, Evas_Object *smart);

/// Uploads `patch` into `image` and sets its border.
void setNinePatch(Evas_Object *image, const NinePatch &patch);

} // namespace rntizen
