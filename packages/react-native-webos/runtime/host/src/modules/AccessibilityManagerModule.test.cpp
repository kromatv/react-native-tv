#include "AccessibilityManagerModule.h"

#include "../platform/FakeLuna.test.h"
#include "../threading/MainLoop.test.h"
#include "TurboModuleTest.test.h"

#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

using facebook::jsi::String;
using testing::call;
using testing::evaluate;

constexpr const char *kSettings = "luna://com.webos.settingsservice/getSystemSettings";
constexpr const char *kGuidanceOff =
    R"({"subscribed":false,"category":"option","method":"getSystemSettings","settings":{"audioGuidance":"off"},"returnValue":true})";
constexpr const char *kGuidanceOn =
    R"({"category":"option","method":"getSystemSettings","settings":{"audioGuidance":"on"},"returnValue":true})";

} // namespace

TEST_CASE("every accessibility state but the screen reader answers off at once, and the setters do nothing") {
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
       }) {
    call(rt, module, state, testing::recorder(rt, successes), testing::recorder(rt, failures));
  }

  CHECK(*successes == std::vector<std::string>(7, "false"));
  CHECK(failures->empty());
  CHECK(call(rt, module, "setAccessibilityContentSizeMultipliers", evaluate(rt, "({extraLarge: 2})")).isUndefined());
  CHECK(call(rt, module, "setAccessibilityFocus", 4).isUndefined());
  CHECK(call(rt, module, "announceForAccessibility", String::createFromUtf8(rt, "hi")).isUndefined());
  CHECK(call(rt, module, "announceForAccessibilityWithOptions", String::createFromUtf8(rt, "hi"), evaluate(rt, "({})"))
            .isUndefined());
}

TEST_CASE("the screen reader state is the TV's audio guidance setting, and off without a TV to ask") {
  const testing::MainLoopScope mainLoop;
  auto runtime = testing::makeRuntime();
  auto &rt = *runtime;
  auto invoker = std::make_shared<testing::QueuedInvoker>();
  auto answers = std::make_shared<std::vector<std::string>>();
  auto failures = std::make_shared<std::vector<std::string>>();
  auto ask = [&](AccessibilityManagerModule &module) {
    call(rt, module, "getCurrentVoiceOverState", testing::recorder(rt, answers), testing::recorder(rt, failures));
    testing::FakeLuna::deliver();
    invoker->run(rt);
  };

  AccessibilityManagerModule alone(invoker, ModuleContext{});
  ask(alone);
  CHECK(*answers == std::vector<std::string>{"false"});

  testing::FakeLuna luna;
  luna.answer(kSettings, kGuidanceOn);
  LunaBus bus(luna.transport());
  ModuleContext context{};
  context.luna = &bus;
  AccessibilityManagerModule onTv(invoker, context);
  ask(onTv);
  luna.answer(kSettings, kGuidanceOff);
  ask(onTv);
  luna.answer(kSettings, R"({"returnValue":false,"errorText":"There is no matched result from DB"})");
  ask(onTv);

  CHECK(*answers == std::vector<std::string>{"false", "true", "false", "false"});
  CHECK(failures->empty());
  REQUIRE(luna.callsTo(kSettings).size() == 3);
  CHECK(luna.callsTo(kSettings)[0] ==
        folly::dynamic::object("category", "option")("keys", folly::dynamic::array("audioGuidance"))("subscribe", false));
}

TEST_CASE("a change of the TV's audio guidance is the screenReaderChanged event") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kSettings, kGuidanceOff);
  LunaBus bus(luna.transport());
  testing::DeviceEvents events;
  ModuleContext context{};
  context.luna = &bus;
  context.emitDeviceEvent = events.emitter();
  {
    const AccessibilityManagerModule module(std::make_shared<testing::QueuedInvoker>(), context);
    testing::FakeLuna::deliver();
    CHECK(luna.isSubscribed(kSettings));
    CHECK(events.names().empty());

    luna.publish(kSettings, kGuidanceOn);
    luna.publish(kSettings, kGuidanceOn);
    luna.publish(kSettings, R"({"settings":{},"returnValue":true})");
    luna.publish(kSettings, kGuidanceOff);

    const auto seen = events.waitFor(2);
    REQUIRE(seen.size() == 2);
    CHECK(seen[0].name == "screenReaderChanged");
    CHECK(seen[0].payload == true);
    CHECK(seen[1].payload == false);
  }
  CHECK_FALSE(luna.isSubscribed(kSettings));
}

TEST_CASE("audioGuidanceOf reads on and off, and nothing from a reply without the setting") {
  CHECK(audioGuidanceOf(folly::parseJson(kGuidanceOn)) == true);
  CHECK(audioGuidanceOf(folly::parseJson(kGuidanceOff)) == false);
  CHECK_FALSE(audioGuidanceOf(folly::parseJson(R"({"settings":{"audioGuidance":1},"returnValue":true})")).has_value());
  CHECK_FALSE(audioGuidanceOf(folly::parseJson(R"({"returnValue":false})")).has_value());
  CHECK_FALSE(audioGuidanceOf(nullptr).has_value());
}

} // namespace rnwebos
