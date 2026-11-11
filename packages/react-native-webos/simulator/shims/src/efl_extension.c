#include <efl_extension.h>

Eina_Bool eext_win_keygrab_set(Elm_Win *obj, const char *key) {
  return obj != NULL && key != NULL;
}

Eina_Bool eext_win_keygrab_unset(Elm_Win *obj, const char *key) {
  return obj != NULL && key != NULL;
}
