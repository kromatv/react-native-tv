#include "NextFrame.h"

#include "threading/MainLoop.test.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

#include <atomic>
#include <utility>

namespace rntizen::expo {

namespace {

struct Canvas {
  Canvas() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    canvas = ecore_evas_buffer_new(64, 36);
    window = evas_object_rectangle_add(ecore_evas_get(canvas));
    evas_object_resize(window, 64, 36);
    evas_object_show(window);
    ecore_evas_manual_render(canvas);
  }

  ~Canvas() {
    ecore_evas_free(canvas);
  }

  Canvas(const Canvas &) = delete;
  Canvas &operator=(const Canvas &) = delete;

  Ecore_Evas *canvas;
  Evas_Object *window;
};

} // namespace

TEST_CASE("afterNextFrame runs once the window's canvas has drawn, and only once") {
  const testing::MainLoopScope loop;
  const Canvas screen;
  int runs = 0;
  afterNextFrame(screen.window, [&runs] { ++runs; });
  CHECK(runs == 0);

  evas_object_color_set(screen.window, 0, 0, 0, 255);
  ecore_evas_manual_render(screen.canvas);
  CHECK(runs == 1);

  evas_object_color_set(screen.window, 255, 255, 255, 255);
  ecore_evas_manual_render(screen.canvas);
  CHECK(runs == 1);
}

TEST_CASE("afterNextFrame asked while a frame is being drawn waits for the one after") {
  struct Ask {
    Evas_Object *window;
    int runs{0};
    bool asked{false};
  };
  const testing::MainLoopScope loop;
  const Canvas screen;
  Ask ask{.window = screen.window};
  evas_event_callback_add(
      ecore_evas_get(screen.canvas),
      EVAS_CALLBACK_RENDER_FLUSH_PRE,
      [](void *data, Evas *, void *) {
        auto *asking = static_cast<Ask *>(data);
        if (std::exchange(asking->asked, true)) return;
        afterNextFrame(asking->window, [asking] { ++asking->runs; });
      },
      &ask);

  evas_object_color_set(screen.window, 0, 0, 0, 255);
  ecore_evas_manual_render(screen.canvas);
  REQUIRE(ask.asked);
  CHECK(ask.runs == 0);

  evas_object_color_set(screen.window, 255, 255, 255, 255);
  ecore_evas_manual_render(screen.canvas);
  CHECK(ask.runs == 1);
}

TEST_CASE("afterNextFrame runs after a tenth of a second when the canvas draws nothing") {
  const testing::MainLoopScope loop;
  const Canvas screen;
  ecore_evas_manual_render_set(screen.canvas, EINA_TRUE);
  std::atomic<bool> ran{false};
  const auto asked = std::chrono::steady_clock::now();
  afterNextFrame(screen.window, [&ran] { ran = true; });
  CHECK_FALSE(ran);

  REQUIRE(loop.runUntil(ran));
  CHECK(std::chrono::steady_clock::now() - asked >= std::chrono::milliseconds(90));
}

TEST_CASE("afterNextFrame runs at once without a window, and when the window goes first") {
  const testing::MainLoopScope loop;
  int runs = 0;
  afterNextFrame(nullptr, [&runs] { ++runs; });
  CHECK(runs == 1);

  const Canvas screen;
  afterNextFrame(screen.window, [&runs] { ++runs; });
  evas_object_del(screen.window);
  CHECK(runs == 2);
}

} // namespace rntizen::expo
