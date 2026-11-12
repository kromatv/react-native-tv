#include "HostWindow.h"

#include "logging/Log.h"

#include <Ecore_Evas.h>
#include <Elementary.h>

#include <cstdlib>

namespace rntizen {

namespace {

constexpr const char *kWindowName = "react-native";

Evas_Object *window_ = nullptr;
Evas_Object *background_ = nullptr;

void forgetWindow(void *, Evas *, Evas_Object *object, void *) {
  if (object != window_) return;
  window_ = nullptr;
  background_ = nullptr;
}

void logRenderingSetup(Evas *evas) {
  const char *engine = ecore_evas_engine_name_get(ecore_evas_ecore_evas_get(evas));
  const char *fontDpi = std::getenv("EVAS_FONT_DPI");
  auto *fonts = evas_font_available_list(evas);
  std::string families;
  void *family = nullptr;
  Eina_List *node = nullptr;
  EINA_LIST_FOREACH(fonts, node, family) {
    if (!families.empty()) families += ", ";
    families += static_cast<const char *>(family);
  }
  evas_font_available_list_free(evas, fonts);
  log(LogLevel::Info,
      std::string("Rendering with ") + (engine != nullptr ? engine : "an unknown engine") + ", EVAS_FONT_DPI=" +
          (fontDpi != nullptr ? fontDpi : "unset"));
  log(LogLevel::Debug, "Fonts: " + families);
}

} // namespace

Evas_Object *createHostWindow(const std::string &title, int width, int height) {
  elm_config_accel_preference_set("opengl");
  auto *window = elm_win_add(nullptr, kWindowName, ELM_WIN_BASIC);
  elm_win_title_set(window, title.c_str());
  elm_win_autodel_set(window, EINA_FALSE);
  auto *background = evas_object_rectangle_add(evas_object_evas_get(window));
  evas_object_color_set(background, 0, 0, 0, 255);
  evas_object_size_hint_weight_set(background, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  elm_win_resize_object_add(window, background);
  evas_object_show(background);
  evas_object_resize(window, width, height);
  evas_object_show(window);
  logRenderingSetup(evas_object_evas_get(window));
  window_ = window;
  background_ = background;
  evas_object_event_callback_add(window, EVAS_CALLBACK_DEL, forgetWindow, nullptr);
  return window;
}

Evas_Object *hostWindow() {
  return window_;
}

void setHostWindowTransparent(bool transparent) {
  if (window_ == nullptr) return;
  elm_win_alpha_set(window_, transparent ? EINA_TRUE : EINA_FALSE);
  log(LogLevel::Info, std::string("Window alpha ") + (elm_win_alpha_get(window_) == EINA_TRUE ? "on" : "off"));
}

} // namespace rntizen
