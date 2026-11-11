#pragma once

#include <react/renderer/core/EventBeat.h>

namespace rntizen {

/// Delivers events as soon as they are queued. Android waits for the next
/// frame and iOS for the run loop to idle; a TV's input is slow enough that
/// batching gains nothing.
class InducingEventBeat final : public facebook::react::EventBeat {
 public:
  using EventBeat::EventBeat;

  void request() const override;
};

} // namespace rntizen
