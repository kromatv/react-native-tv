#pragma once

#include <Evas.h>

#include <string_view>

namespace rntizen {

/// Whether this Evas runs filter programs. `efl_gfx_filter_program_set` and
/// `efl_gfx_filter_data_set` live in libefl.so.1, which the Tizen rootstraps
/// neither ship nor declare outside the beta API, so they are resolved at run
/// time, once (efl-rendering.md, "Backdrop blur").
bool evasFiltersAvailable();

/// Runs `code`, an Evas filter program, whenever `object` (an image or a text)
/// renders; null removes it. Does nothing when filters are unavailable.
void setEvasFilterProgram(Evas_Object *object, const char *code);

/// Sets the global `name` to `value` in `object`'s filter program, before or
/// after the program itself.
void setEvasFilterNumber(Evas_Object *object, std::string_view name, int value);

} // namespace rntizen
