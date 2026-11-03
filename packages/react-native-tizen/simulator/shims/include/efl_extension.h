#pragma once

#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Succeeds without grabbing: the simulator's window always has the keyboard
/// focus, so every key reaches it already.
Eina_Bool eext_win_keygrab_set(Evas_Object *obj, const char *key);

Eina_Bool eext_win_keygrab_unset(Evas_Object *obj, const char *key);

#ifdef __cplusplus
}
#endif
