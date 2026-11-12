#include "ScreenSaver.h"

#include "../threading/MainLoop.test.h"
#include "FakeLuna.test.h"

#include <SDL3/SDL.h>
#include <doctest/doctest.h>

namespace rnwebos {

namespace {

constexpr const char *kRegister = "luna://com.webos.service.tvpower/power/registerScreenSaverRequest";
constexpr const char *kRespond = "luna://com.webos.service.tvpower/power/responseScreenSaverRequest";

} // namespace

TEST_CASE("a screen saver request is answered with its own timestamp and whether the saver may start") {
  const auto request = folly::parseJson(R"({"state":"Active","timestamp":"7f3a1c","returnValue":true})");

  CHECK(*screenSaverAnswer(request, "org.rnwebos.example.wakelock", true) ==
        folly::dynamic::object("clientName", "org.rnwebos.example.wakelock")("ack", true)("timestamp", "7f3a1c"));
  CHECK((*screenSaverAnswer(request, "org.rnwebos.example.wakelock", false))["ack"] == false);
  CHECK_FALSE(screenSaverAnswer(folly::parseJson(R"({"subscribed":true,"returnValue":true})"), "x", true).has_value());
  CHECK_FALSE(screenSaverAnswer(nullptr, "x", true).has_value());
}

TEST_CASE("the screen saver may start unless SDL's screen saver is disabled, as while keep awake is active") {
  const testing::MainLoopScope mainLoop;
  REQUIRE(SDL_InitSubSystem(SDL_INIT_VIDEO));
  testing::FakeLuna luna;
  luna.answer(kRegister, R"({"subscribed":true,"returnValue":true})");
  LunaBus bus(luna.transport());

  const auto saver = ScreenSaver::watch(bus, "org.rnwebos.example");
  REQUIRE(saver != nullptr);
  testing::FakeLuna::deliver();
  REQUIRE(luna.callsTo(kRegister).size() == 1);
  CHECK(luna.callsTo(kRegister)[0] ==
        folly::dynamic::object("subscribe", true)("clientName", "org.rnwebos.example.wakelock"));
  CHECK(luna.callsTo(kRespond).empty());

  SDL_EnableScreenSaver();
  luna.publish(kRegister, R"({"timestamp":"1","returnValue":true})");
  SDL_DisableScreenSaver();
  luna.publish(kRegister, R"({"timestamp":"2","returnValue":true})");
  SDL_EnableScreenSaver();

  const auto answers = luna.callsTo(kRespond);
  REQUIRE(answers.size() == 2);
  CHECK(answers[0] == folly::dynamic::object("clientName", "org.rnwebos.example.wakelock")("ack", true)("timestamp", "1"));
  CHECK(answers[1]["ack"] == false);
  CHECK(answers[1]["timestamp"] == "2");
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

TEST_CASE("nothing watches the screen saver without a bus") {
  testing::FakeLuna luna;
  luna.setAvailable(false);
  LunaBus bus(luna.transport());

  CHECK(ScreenSaver::watch(bus, "org.rnwebos.example") == nullptr);
}

} // namespace rnwebos
