#include "AppRegistration.h"

#include "../threading/MainLoop.test.h"
#include "FakeLuna.test.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rnwebos {

namespace {

constexpr const char *kRegisterApp = "luna://com.webos.applicationManager/registerApp";
constexpr const char *kRegisterNativeApp = "luna://com.webos.applicationManager/registerNativeApp";
constexpr const char *kAppId = "org.rnwebos.example";

struct Seen {
  std::vector<std::string> events;

  AppRegistration::Events handlers() {
    return {
        .relaunch =
            [this](const folly::dynamic &parameters, const std::string &reason) {
              events.push_back("relaunch " + folly::toJson(parameters) + " " + reason);
            },
        .pause = [this] { events.emplace_back("pause"); },
        .close = [this] { events.emplace_back("close"); },
    };
  }
};

} // namespace

TEST_CASE("lifecycle interface 2 registers with registerApp and reads what happened from `event`") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kRegisterApp, R"({"returnValue":true,"event":"registered"})");
  LunaBus bus(luna.transport());
  Seen seen;

  const auto registration = AppRegistration::start(bus, kAppId, 2, seen.handlers());
  REQUIRE(registration != nullptr);
  testing::FakeLuna::deliver();
  REQUIRE(luna.calls().size() == 1);
  CHECK(luna.calls()[0].uri == kRegisterApp);
  CHECK(luna.calls()[0].payload == folly::dynamic::object("id", kAppId));
  CHECK(seen.events.empty());

  luna.publish(
      kRegisterApp,
      R"({"event":"relaunch","reason":"launcher","appId":"org.rnwebos.example",)"
      R"("parameters":{"contentTarget":"example://title/42"},"returnValue":true})");
  luna.publish(kRegisterApp, R"({"event":"pause","reason":"keepAlive","parameters":{},"returnValue":true})");
  luna.publish(kRegisterApp, R"({"event":"close","reason":"memoryReclaim","returnValue":true})");

  CHECK(seen.events == std::vector<std::string>{
                           R"(relaunch {"contentTarget":"example://title/42"} launcher)",
                           "pause",
                           "close",
                       });
}

TEST_CASE("lifecycle interface 1 registers with registerNativeApp and reads what happened from `message`") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kRegisterNativeApp, R"({"returnValue":true,"message":"registered"})");
  LunaBus bus(luna.transport());
  Seen seen;

  const auto registration = AppRegistration::start(bus, kAppId, 1, seen.handlers());
  REQUIRE(registration != nullptr);
  testing::FakeLuna::deliver();
  luna.publish(kRegisterNativeApp, R"({"message":"relaunch","parameters":{"customParam1":"value1"},"returnValue":true})");
  luna.publish(kRegisterNativeApp, R"({"message":"relaunch","returnValue":true})");

  CHECK(luna.callsTo(kRegisterApp).empty());
  CHECK(seen.events == std::vector<std::string>{R"(relaunch {"customParam1":"value1"} )", "relaunch {} "});
}

TEST_CASE("a registration under the wrong lifecycle interface is made again under the other one, once") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(
      kRegisterApp,
      R"({"returnValue":false,"errorCode":-1,)"
      R"("errorText":"trying to register via unmatched method with nativeLifeCycleInterfaceVersion"})");
  luna.answer(kRegisterNativeApp, R"({"returnValue":true,"message":"registered"})");
  LunaBus bus(luna.transport());
  Seen seen;

  const auto registration = AppRegistration::start(bus, kAppId, 2, seen.handlers());
  testing::FakeLuna::deliver();
  testing::FakeLuna::deliver();

  CHECK(luna.callsTo(kRegisterApp).size() == 1);
  CHECK(luna.callsTo(kRegisterNativeApp).size() == 1);
  CHECK_FALSE(luna.isSubscribed(kRegisterApp));
  luna.publish(kRegisterNativeApp, R"({"message":"relaunch","parameters":{},"returnValue":true})");
  CHECK(seen.events == std::vector<std::string>{"relaunch {} "});
}

TEST_CASE("an app manager that refuses both lifecycle interfaces is not asked a third time") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kRegisterApp, R"({"returnValue":false,"errorText":"org.rnwebos.example is not running"})");
  luna.answer(kRegisterNativeApp, R"({"returnValue":false,"errorText":"org.rnwebos.example is not running"})");
  LunaBus bus(luna.transport());
  Seen seen;

  const auto registration = AppRegistration::start(bus, kAppId, 2, seen.handlers());
  for (int round = 0; round < 4; ++round) testing::FakeLuna::deliver();

  CHECK(luna.calls().size() == 2);
  CHECK(seen.events.empty());
}

TEST_CASE("there is no registration without a bus, and none is left once it is destroyed") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  LunaBus bus(luna.transport());
  Seen seen;

  auto registration = AppRegistration::start(bus, kAppId, 2, seen.handlers());
  REQUIRE(registration != nullptr);
  CHECK(luna.isSubscribed(kRegisterApp));
  registration.reset();
  CHECK_FALSE(luna.isSubscribed(kRegisterApp));

  luna.setAvailable(false);
  CHECK(AppRegistration::start(bus, kAppId, 2, seen.handlers()) == nullptr);
}

} // namespace rnwebos
