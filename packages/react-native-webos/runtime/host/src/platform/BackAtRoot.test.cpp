#include "BackAtRoot.h"

#include "../threading/MainLoop.test.h"
#include "FakeLuna.test.h"

#include <doctest/doctest.h>

namespace rnwebos {

namespace {

constexpr const char *kLaunch = "luna://com.webos.applicationManager/launch";

} // namespace

TEST_CASE("Back at the root asks the app manager for Home, and the app keeps running behind it") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(kLaunch, R"({"returnValue":true})");
  LunaBus bus(luna.transport());
  int exits = 0;

  leaveOnBackAtRoot(bus, BackAtRoot::ShowHome, [&exits] { ++exits; });
  testing::FakeLuna::deliver();

  REQUIRE(luna.calls().size() == 1);
  CHECK(luna.calls()[0].uri == kLaunch);
  CHECK(luna.calls()[0].payload ==
        folly::dynamic::object("id", "com.webos.app.home")("params", folly::dynamic::object("launchType", "homeKey")));
  CHECK(exits == 0);
}

TEST_CASE("Back at the root ends the app when the app manager will not open Home") {
  const testing::MainLoopScope mainLoop;
  testing::FakeLuna luna;
  luna.answer(
      kLaunch,
      R"({"returnValue":false,"errorCode":-104,)"
      R"("errorText":"Cannot launch a privileged app on development mode from developer apps path"})");
  LunaBus bus(luna.transport());
  int exits = 0;

  leaveOnBackAtRoot(bus, BackAtRoot::ShowHome, [&exits] { ++exits; });
  CHECK(exits == 0);
  testing::FakeLuna::deliver();
  CHECK(exits == 1);

  luna.setAvailable(false);
  leaveOnBackAtRoot(bus, BackAtRoot::ShowHome, [&exits] { ++exits; });
  CHECK(exits == 2);
}

TEST_CASE("the Exit behaviour ends the app without asking the TV anything") {
  testing::FakeLuna luna;
  LunaBus bus(luna.transport());
  int exits = 0;

  leaveOnBackAtRoot(bus, BackAtRoot::Exit, [&exits] { ++exits; });

  CHECK(exits == 1);
  CHECK(luna.calls().empty());
}

} // namespace rnwebos
