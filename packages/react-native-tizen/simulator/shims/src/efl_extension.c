#include <efl_extension.h>

Eina_Bool eext_win_keygrab_set(Evas_Object *obj, const char *key) {
  return obj != NULL && key != NULL;
}

Eina_Bool eext_win_keygrab_unset(Evas_Object *obj, const char *key) {
  return obj != NULL && key != NULL;
}
