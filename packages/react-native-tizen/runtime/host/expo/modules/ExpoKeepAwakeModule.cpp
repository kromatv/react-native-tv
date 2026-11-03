#include "../ExpoModuleRegistry.h"
#include "../ModuleDefinition.h"

#include "logging/Log.h"

#include <Ecore_Evas.h>
#include <Elementary.h>

#include <set>

namespace rntizen::expo {

namespace {

Evas_Object *appWindow() {
  Evas_Object *window = nullptr;
  auto *canvases = ecore_evas_ecore_evas_list_get();
  void *canvas = nullptr;
  Eina_List *node = nullptr;
  EINA_LIST_FOREACH(canvases, node, canvas) {
    auto *candidate = static_cast<Evas_Object *>(ecore_evas_data_get(static_cast<Ecore_Evas *>(canvas), "elm_win"));
    if (candidate != nullptr && window == nullptr) window = candidate;
  }
  eina_list_free(canvases);
  return window;
}

class ExpoKeepAwakeModule final : public Module {
 public:
  ModuleDefinition definition() override {
    return ModuleDefinition("ExpoKeepAwake")
        .asyncFunction("activate", [this](const std::string &tag) { return update(tag, true); }, Queue::Main)
        .asyncFunction("deactivate", [this](const std::string &tag) { return update(tag, false); }, Queue::Main)
        .asyncFunction("isActivated", [this] { return !tags_.empty(); }, Queue::Main)
        .asyncFunction("isAvailableAsync", [] { return appWindow() != nullptr; }, Queue::Main);
  }

 private:
  bool update(const std::string &tag, bool active) {
    if (active) {
      tags_.insert(tag);
    } else {
      tags_.erase(tag);
    }
    auto *window = appWindow();
    if (window == nullptr) {
      log(LogLevel::Warning, "ExpoKeepAwake found no window to keep awake");
      return false;
    }
    elm_win_noblank_set(window, tags_.empty() ? EINA_FALSE : EINA_TRUE);
    log(LogLevel::Debug,
        std::string("Keep awake ") + (elm_win_noblank_get(window) == EINA_TRUE ? "on" : "off") + " (" +
            std::to_string(tags_.size()) + " tags)");
    return true;
  }

  std::set<std::string, std::less<>> tags_;
};

} // namespace

RN_TIZEN_EXPO_MODULE(ExpoKeepAwake, ExpoKeepAwakeModule);

} // namespace rntizen::expo
