#include "ScreenSize.h"

#include "../HostWindow.h"
#include "../logging/Log.h"

#include <Ecore_Evas.h>
#include <Elementary.h>
#include <dlfcn.h>

#include <bit>
#include <format>
#include <string>

namespace rntizen {

namespace {

constexpr int kUhdWidth = 3840;
constexpr int kUhdHeight = 2160;

std::string text(PixelSize size) {
  return std::format("{}x{}", size.width, size.height);
}

PixelSize windowManagerScreen(Evas_Object *window) {
  int x = 0;
  int y = 0;
  PixelSize size{0, 0};
  elm_win_screen_size_get(window, &x, &y, &size.width, &size.height);
  return size;
}

PixelSize ecoreEvasScreen(Evas_Object *window) {
  int x = 0;
  int y = 0;
  PixelSize size{0, 0};
  auto *ecoreEvas = ecore_evas_ecore_evas_get(evas_object_evas_get(window));
  if (ecoreEvas != nullptr) ecore_evas_screen_geometry_get(ecoreEvas, &x, &y, &size.width, &size.height);
  return size;
}

Eina_Bool logSettledSurface(void *) {
  auto *window = hostWindow();
  if (window == nullptr) return ECORE_CALLBACK_CANCEL;
  PixelSize geometry{0, 0};
  PixelSize canvas{0, 0};
  evas_object_geometry_get(window, nullptr, nullptr, &geometry.width, &geometry.height);
  evas_output_size_get(evas_object_evas_get(window), &canvas.width, &canvas.height);
  log(LogLevel::Info, "Screen: once shown, window " + text(geometry) + ", canvas " + text(canvas));
  return ECORE_CALLBACK_CANCEL;
}

// Ecore_Wl2 is resolved at run time: the simulator draws through X11, and the
// all-screens call is Tizen's own (docs/reference/tizen-tv-platform.md, section 7).
std::string waylandScreens() {
  using ConnectedDisplay = void *(*)(const char *);
  using SizeGet = void (*)(void *, int *, int *);
  auto connected = std::bit_cast<ConnectedDisplay>(dlsym(RTLD_DEFAULT, "ecore_wl2_connected_display_get"));
  auto screen = std::bit_cast<SizeGet>(dlsym(RTLD_DEFAULT, "ecore_wl2_display_screen_size_get"));
  auto all = std::bit_cast<SizeGet>(dlsym(RTLD_DEFAULT, "ecore_wl2_display_all_screens_size_get"));
  void *display = connected != nullptr ? connected(nullptr) : nullptr;
  if (display == nullptr) return "";
  PixelSize one{0, 0};
  PixelSize every{0, 0};
  if (screen != nullptr) screen(display, &one.width, &one.height);
  if (all != nullptr) all(display, &every.width, &every.height);
  return ", ecore_wl2_display_screen_size_get " + text(one) + ", ecore_wl2_display_all_screens_size_get " + text(every);
}

} // namespace

PixelSize fitWindowToScreen(Evas_Object *window, bool uhd, PixelSize systemInfo) {
  const auto screen = windowManagerScreen(window);
  const auto ecoreScreen = ecoreEvasScreen(window);
  const bool uhdScreen = screen.width >= kUhdWidth && screen.height >= kUhdHeight;
  const auto size = uhd && uhdScreen ? screen : systemInfo;
  if (size.width != systemInfo.width || size.height != systemInfo.height) {
    evas_object_resize(window, size.width, size.height);
  }
  PixelSize canvas{0, 0};
  evas_output_size_get(evas_object_evas_get(window), &canvas.width, &canvas.height);
  std::string request;
  if (uhd) request = uhdScreen ? " (asked for a 4K UI)" : " (asked for a 4K UI, the screen is not 4K)";
  log(LogLevel::Info,
      "Screen: system_info " + text(systemInfo) + ", elm_win_screen_size_get " + text(screen) +
          ", ecore_evas_screen_geometry_get " + text(ecoreScreen) + waylandScreens() + "; window " + text(size) + ", canvas " +
          text(canvas) + request);
  ecore_timer_add(2.0, logSettledSurface, nullptr);
  return size;
}

} // namespace rntizen
