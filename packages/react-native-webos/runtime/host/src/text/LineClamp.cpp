#include "LineClamp.h"

#include "../Malloced.h"

#include <string>

namespace rntizen {

namespace {

constexpr const char *kEllipsis = "…";

bool hasLine(Evas_Object *textblock, int line) {
  return evas_object_textblock_line_number_geometry_get(textblock, line, nullptr, nullptr, nullptr, nullptr) == EINA_TRUE;
}

std::string contentAt(const Evas_Textblock_Cursor *cursor) {
  const Malloced<char> content(evas_textblock_cursor_content_get(cursor));
  return content == nullptr ? std::string() : std::string(content.get());
}

void appendEllipsis(Evas_Textblock_Cursor *end) {
  if (!evas_textblock_cursor_char_prev(end)) {
    evas_textblock_cursor_text_prepend(end, kEllipsis);
    return;
  }
  if (evas_textblock_cursor_format_is_visible_get(end)) {
    evas_textblock_cursor_char_next(end);
    evas_textblock_cursor_text_prepend(end, kEllipsis);
    return;
  }
  evas_textblock_cursor_text_prepend(end, (contentAt(end) + kEllipsis).c_str());
  evas_textblock_cursor_char_delete(end);
}

void trimTrailingSpaces(Evas_Textblock_Cursor *end) {
  while (evas_textblock_cursor_char_prev(end)) {
    if (contentAt(end) != " ") {
      evas_textblock_cursor_char_next(end);
      return;
    }
    evas_textblock_cursor_char_delete(end);
  }
}

} // namespace

void clampLines(Evas_Object *textblock, int lines, bool ellipsis) {
  if (lines <= 0 || !hasLine(textblock, lines)) return;
  auto *cut = evas_object_textblock_cursor_new(textblock);
  auto *end = evas_object_textblock_cursor_new(textblock);
  evas_textblock_cursor_line_set(cut, lines - 1);
  evas_textblock_cursor_line_char_last(cut);
  evas_textblock_cursor_paragraph_last(end);
  evas_textblock_cursor_range_delete(cut, end);
  trimTrailingSpaces(cut);
  if (ellipsis) {
    appendEllipsis(cut);
    while (hasLine(textblock, lines) && evas_textblock_cursor_char_prev(cut) && evas_textblock_cursor_char_prev(cut)) {
      evas_textblock_cursor_char_delete(cut);
      evas_textblock_cursor_char_next(cut);
    }
  }
  evas_textblock_cursor_free(end);
  evas_textblock_cursor_free(cut);
}

} // namespace rntizen
