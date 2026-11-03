#include "InducingEventBeat.h"

namespace rntizen {

void InducingEventBeat::request() const {
  EventBeat::request();
  induce();
}

} // namespace rntizen
