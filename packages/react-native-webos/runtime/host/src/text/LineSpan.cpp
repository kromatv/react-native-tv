#include "LineSpan.h"

namespace rntizen {

LineSpan lineSpan(Evas_Object *textblock, int maxLines) {
  LineSpan span;
  Evas_Coord y = 0;
  Evas_Coord height = 0;
  for (int line = 0; maxLines <= 0 || line < maxLines; ++line) {
    if (!evas_object_textblock_line_number_geometry_get(textblock, line, nullptr, &y, nullptr, &height)) break;
    if (line == 0) span.top = y;
    span.bottom = y + height;
  }
  return span;
}

} // namespace rntizen
