#pragma once

/* Declared from the Tizen native API reference, Efl Extension Hardware Keyevent:
 * https://samsungtizenos.com/docs/application/native/api/10.0.0/common/group__EFL__EXTENSION__HARDWARE__KEYEVENT__GROUP */

#include <Elementary.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Succeeds without grabbing: the simulator's window always has the keyboard
/// focus, so every key reaches it already.
Eina_Bool eext_win_keygrab_set(Elm_Win *obj, const char *key);

Eina_Bool eext_win_keygrab_unset(Elm_Win *obj, const char *key);

#ifdef __cplusplus
}
#endif
