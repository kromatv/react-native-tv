#pragma once

#include <Evas.h>

namespace rntizen {

/// Where a textblock's lines start and end, in pixels from its top.
struct LineSpan {
  Evas_Coord top{0};
  Evas_Coord bottom{0};
};

/// The top of the first line and the bottom of the last one, or of line
/// `maxLines` when there are more and `maxLines` is above 0. Unlike the
/// formatted size, it leaves out the room Evas adds for the font's maximum
/// ascent above the first line and maximum descent below the last. Main thread
/// only.
LineSpan lineSpan(Evas_Object *textblock, int maxLines);

} // namespace rntizen
