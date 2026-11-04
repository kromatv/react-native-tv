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

constexpr Ecore_Window kWindowNoCanvasListensOn = 42;

void post(int type, const char *name, unsigned int keycode) {
  auto *key = static_cast<Ecore_Event_Key *>(std::calloc(1, sizeof(Ecore_Event_Key)));
  key->keyname = name;
  key->key = name;
  key->keycode = keycode;
  key->window = kWindowNoCanvasListensOn;
  key->event_window = kWindowNoCanvasListensOn;
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

class EcoreEvas final {
 public:
  EcoreEvas() {
    REQUIRE(ecore_evas_init() > 0);
    REQUIRE(ecore_event_init() > 0);
  }
  ~EcoreEvas() {
    ecore_event_shutdown();
    ecore_evas_shutdown();
  }
  EcoreEvas(const EcoreEvas &) = delete;
  EcoreEvas &operator=(const EcoreEvas &) = delete;
};

class PassedOnKeys final {
 public:
  PassedOnKeys()
      : down_(ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, record, &downs)),
        up_(ecore_event_handler_add(ECORE_EVENT_KEY_UP, record, &ups)) {}
  ~PassedOnKeys() {
    ecore_event_handler_del(up_);
    ecore_event_handler_del(down_);
  }
  PassedOnKeys(const PassedOnKeys &) = delete;
  PassedOnKeys &operator=(const PassedOnKeys &) = delete;

  std::vector<std::string> downs;
  std::vector<std::string> ups;

 private:
  static Eina_Bool record(void *keys, int, void *event) {
    static_cast<std::vector<std::string> *>(keys)->emplace_back(static_cast<const Ecore_Event_Key *>(event)->key);
    return ECORE_CALLBACK_PASS_ON;
  }

  Ecore_Event_Handler *down_;
  Ecore_Event_Handler *up_;
};

RemotePresses::EventHandler recordInto(std::vector<std::string> &events) {
  return [&events](const RemoteEvent &event) {
    events.push_back(std::string(event.eventType) + "/" + std::to_string(event.keyAction));
  };
}

} // namespace

TEST_CASE("RemoteInput turns the window's key events into remote events, and ends holds when the window loses focus") {
  const EcoreEvas ecore;
  auto *canvas = ecore_evas_buffer_new(100, 100);
  auto *window = evas_object_box_add(ecore_evas_get(canvas));
  std::vector<std::string> events;
  bool fieldTakesKeys = false;

  {
    RemoteInput input(window, recordInto(events), [&fieldTakesKeys](const Ecore_Event_Key &, bool) {
      return fieldTakesKeys;
    });

    post(ECORE_EVENT_KEY_DOWN, "Return", 36);
    post(ECORE_EVENT_KEY_UP, "Return", 36);
    post(ECORE_EVENT_KEY_DOWN, "Up", 111);
    runMainLoopFor(0.4);
    evas_object_smart_callback_call(window, "focus,out", nullptr);
  }

  CHECK(events == std::vector<std::string>{"select/1", "longUp/0", "longUp/1"});
  evas_object_del(window);
  ecore_evas_free(canvas);
}

TEST_CASE("RemoteInput passes on a key with no remote meaning and a release it never saw pressed") {
  const EcoreEvas ecore;
  auto *canvas = ecore_evas_buffer_new(100, 100);
  auto *window = evas_object_box_add(ecore_evas_get(canvas));
  std::vector<std::string> events;

  {
    const RemoteInput input(window, recordInto(events));
    const PassedOnKeys passedOn;

    post(ECORE_EVENT_KEY_DOWN, "F13", 191);
    post(ECORE_EVENT_KEY_UP, "F13", 191);
    post(ECORE_EVENT_KEY_DOWN, "XF86AudioPlay", 172);
    post(ECORE_EVENT_KEY_UP, "XF86AudioPlay", 172);
    post(ECORE_EVENT_KEY_UP, "Down", 116);
    runMainLoopFor(0.05);

    CHECK(passedOn.downs == std::vector<std::string>{"F13"});
    CHECK(passedOn.ups == std::vector<std::string>{"F13", "Down"});
  }

  CHECK(events == std::vector<std::string>{"play/1"});
  evas_object_del(window);
  ecore_evas_free(canvas);
}

TEST_CASE("RemoteInput on a canvas with no native window hears the keys of every window") {
  const EcoreEvas ecore;
  auto *evas = evas_new();
  auto *window = evas_object_box_add(evas);
  std::vector<std::string> events;

  {
    const RemoteInput input(window, recordInto(events));

    post(ECORE_EVENT_KEY_DOWN, "XF86AudioPlay", 172);
    post(ECORE_EVENT_KEY_UP, "XF86AudioPlay", 172);
    runMainLoopFor(0.05);
  }

  CHECK(events == std::vector<std::string>{"play/1"});
  evas_object_del(window);
  evas_free(evas);
}

} // namespace rntizen
