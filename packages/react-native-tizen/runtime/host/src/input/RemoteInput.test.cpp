#include "RemoteInput.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Input.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace rntizen {

namespace {

void post(int type, const char *name, unsigned int keycode) {
  auto *key = static_cast<Ecore_Event_Key *>(std::calloc(1, sizeof(Ecore_Event_Key)));
  key->keyname = name;
  key->key = name;
  key->keycode = keycode;
  ecore_event_add(type, key, nullptr, nullptr);
}

void runMainLoopFor(double seconds) {
  ecore_timer_add(
      seconds,
      [](void *) -> Eina_Bool {
        ecore_main_loop_quit();
        return ECORE_CALLBACK_CANCEL;
      },
      nullptr);
  ecore_main_loop_begin();
}

} // namespace

TEST_CASE("RemoteInput turns the window's key events into remote events, and ends holds when the window loses focus") {
  REQUIRE(ecore_evas_init() > 0);
  REQUIRE(ecore_event_init() > 0);
  auto *canvas = ecore_evas_buffer_new(100, 100);
  auto *window = evas_object_box_add(ecore_evas_get(canvas));
  std::vector<std::string> events;
  bool fieldTakesKeys = false;

  {
    RemoteInput input(
        window,
        [&events](const RemoteEvent &event) {
          events.push_back(std::string(event.eventType) + "/" + std::to_string(event.keyAction));
        },
        [&fieldTakesKeys](const Ecore_Event_Key &, bool) { return fieldTakesKeys; });

    post(ECORE_EVENT_KEY_DOWN, "Return", 36);
    post(ECORE_EVENT_KEY_UP, "Return", 36);
    post(ECORE_EVENT_KEY_DOWN, "Up", 111);
    runMainLoopFor(0.4);
    evas_object_smart_callback_call(window, "focus,out", nullptr);
  }

  CHECK(events == std::vector<std::string>{"select/1", "longUp/0", "longUp/1"});
  evas_object_del(window);
  ecore_evas_free(canvas);
  ecore_event_shutdown();
  ecore_evas_shutdown();
}

} // namespace rntizen
