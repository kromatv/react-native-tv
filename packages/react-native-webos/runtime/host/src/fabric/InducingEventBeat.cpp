#include "InducingEventBeat.h"

namespace rnwebos {

void InducingEventBeat::request() const {
  EventBeat::request();
  induce();
}

} // namespace rnwebos
