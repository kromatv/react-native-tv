#include "InducingEventBeat.h"

#include <doctest/doctest.h>

#include <jsi/jsi.h>
#include <react/renderer/runtimescheduler/RuntimeScheduler.h>

#include <functional>
#include <memory>

namespace rntizen {

TEST_CASE("InducingEventBeat schedules its beat on the runtime as soon as one is requested") {
  int scheduled = 0;
  facebook::react::RuntimeScheduler scheduler(
      [&scheduled](std::function<void(facebook::jsi::Runtime &)> &&) { ++scheduled; });
  const auto owner = std::make_shared<int>(0);
  auto ownerBox = std::make_shared<facebook::react::EventBeat::OwnerBox>();
  ownerBox->owner = owner;
  InducingEventBeat beat(ownerBox, scheduler);
  beat.setBeatCallback([](facebook::jsi::Runtime &) {});

  beat.request();

  CHECK(scheduled == 1);
}

} // namespace rntizen
