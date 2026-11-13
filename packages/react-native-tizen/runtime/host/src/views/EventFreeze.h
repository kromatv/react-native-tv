#pragma once

#include <Evas.h>

namespace rntizen {

/// Freezes the canvas's input events while many objects move. Unfrozen, Evas
/// works out at every single move which objects the pointer entered or left
/// and recalculates every clip under a moved clipper; frozen, it does both
/// once, when the freeze ends and at the next frame (efl-rendering.md
/// section 2).
class EventFreeze final {
 public:
  explicit EventFreeze(Evas *evas) : evas_(evas) {
    evas_event_freeze(evas_);
  }
  ~EventFreeze() {
    evas_event_thaw(evas_);
    evas_event_thaw_eval(evas_);
  }

  EventFreeze(const EventFreeze &) = delete;
  EventFreeze &operator=(const EventFreeze &) = delete;

 private:
  Evas *const evas_;
};

} // namespace rntizen
