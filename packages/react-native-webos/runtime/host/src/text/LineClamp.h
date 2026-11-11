#pragma once

#include <Evas.h>

namespace rntizen {

/// Cuts a wrapping textblock's text after `lines` lines, and ends the last line
/// with an ellipsis when `ellipsis` is set. Evas's own ellipsis can cut a
/// wrapping paragraph after its first line whatever its height
/// (efl-rendering.md section 5), so `numberOfLines > 1` is clamped here.
/// Main thread only.
void clampLines(Evas_Object *textblock, int lines, bool ellipsis);

} // namespace rntizen
