#include "NextFrame.h"

#include <Ecore.h>

#include <memory>
#include <utility>

namespace rntizen::expo {

namespace {

constexpr double kNoFrameSeconds = 0.1;

struct FrameWait {
  Evas_Object *window;
  std::function<void()> then;
  Ecore_Timer *timeout{nullptr};
  bool began{false};
};

void onFrameBegins(void *data, Evas *, void *);
void onFrameDrawn(void *data, Evas *, void *);
void onWindowDeleted(void *data, Evas *, Evas_Object *, void *);

void finish(FrameWait *wait) {
  const std::unique_ptr<FrameWait> owned(wait);
  auto *evas = evas_object_evas_get(wait->window);
  evas_event_callback_del_full(evas, EVAS_CALLBACK_RENDER_PRE, onFrameBegins, wait);
  evas_event_callback_del_full(evas, EVAS_CALLBACK_RENDER_POST, onFrameDrawn, wait);
  evas_object_event_callback_del_full(wait->window, EVAS_CALLBACK_DEL, onWindowDeleted, wait);
  if (wait->timeout != nullptr) ecore_timer_del(wait->timeout);
  wait->then();
}

void onFrameBegins(void *data, Evas *, void *) {
  static_cast<FrameWait *>(data)->began = true;
}

// A software engine renders on a thread: the RENDER_POST of a frame begun before
// the wait arrives after it, without what changed since (efl-rendering.md, section 7).
void onFrameDrawn(void *data, Evas *, void *) {
  auto *wait = static_cast<FrameWait *>(data);
  if (wait->began) finish(wait);
}

void onWindowDeleted(void *data, Evas *, Evas_Object *, void *) {
  finish(static_cast<FrameWait *>(data));
}

Eina_Bool onNoFrame(void *data) {
  auto *wait = static_cast<FrameWait *>(data);
  wait->timeout = nullptr;
  finish(wait);
  return ECORE_CALLBACK_CANCEL;
}

} // namespace

void afterNextFrame(Evas_Object *window, std::function<void()> then) {
  if (window == nullptr) {
    then();
    return;
  }
  auto *wait = std::make_unique<FrameWait>(FrameWait{.window = window, .then = std::move(then)}).release();
  wait->timeout = ecore_timer_add(kNoFrameSeconds, onNoFrame, wait);
  auto *evas = evas_object_evas_get(window);
  evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_PRE, onFrameBegins, wait);
  evas_event_callback_add(evas, EVAS_CALLBACK_RENDER_POST, onFrameDrawn, wait);
  evas_object_event_callback_add(window, EVAS_CALLBACK_DEL, onWindowDeleted, wait);
}

} // namespace rntizen::expo
