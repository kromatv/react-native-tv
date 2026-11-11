#include "AccessibilityManagerModule.h"

#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

namespace rntizen {

using facebook::jsi::String;
using testing::call;
using testing::evaluate;

TEST_CASE("every accessibility state answers off through its success callback, and the setters do nothing") {
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  AccessibilityManagerModule module(std::make_shared<testing::QueuedInvoker>(), ModuleContext{});
  auto successes = std::make_shared<std::vector<std::string>>();
  auto failures = std::make_shared<std::vector<std::string>>();

  for (const auto *state : {
           "getCurrentBoldTextState",
           "getCurrentGrayscaleState",
           "getCurrentInvertColorsState",
           "getCurrentReduceMotionState",
           "getCurrentDarkerSystemColorsState",
           "getCurrentPrefersCrossFadeTransitionsState",
           "getCurrentReduceTransparencyState",
           "getCurrentVoiceOverState",
       }) {
    call(rt, module, state, testing::recorder(rt, successes), testing::recorder(rt, failures));
  }

  CHECK(*successes == std::vector<std::string>(8, "false"));
  CHECK(failures->empty());
  CHECK(call(rt, module, "setAccessibilityContentSizeMultipliers", evaluate(rt, "({extraLarge: 2})")).isUndefined());
  CHECK(call(rt, module, "setAccessibilityFocus", 4).isUndefined());
  CHECK(call(rt, module, "announceForAccessibility", String::createFromUtf8(rt, "hi")).isUndefined());
  CHECK(call(rt, module, "announceForAccessibilityWithOptions", String::createFromUtf8(rt, "hi"), evaluate(rt, "({})"))
            .isUndefined());
}

} // namespace rntizen
