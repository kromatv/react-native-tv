#include "LaunchParameters.h"

#include <doctest/doctest.h>

#include <array>
#include <string>

namespace rnwebos {

TEST_CASE("a lifecycle interface 2 launch argument carries the caller's parameters under `parameters`") {
  const auto launch = parseLaunchArgument(
      R"({"event":"launch","reason":"launcher","appId":"org.rnwebos.example","interfaceVersion":2,)"
      R"("interfaceMethod":"registerApp","parameters":{"contentTarget":"example://title/42"},"@system_native_app":true})");

  CHECK(launch.parameters == folly::dynamic::object("contentTarget", "example://title/42"));
  CHECK(launch.reason == "launcher");
  CHECK(launch.interfaceVersion == 2);
}

TEST_CASE("a lifecycle interface 1 launch argument is the caller's parameters plus the app manager's keys") {
  const auto launch = parseLaunchArgument(
      R"({"customParam1":"value1","customParam2":"value2","nid":"org.rnwebos.example","@system_native_app":true,"preload":"full"})");

  CHECK(launch.parameters == folly::dynamic::object("customParam1", "value1")("customParam2", "value2"));
  CHECK(launch.reason.empty());
  CHECK_FALSE(launch.interfaceVersion.has_value());
}

TEST_CASE("the argument of today's open source app manager has the parameters and the launch event side by side") {
  const auto launch = parseLaunchArgument(
      R"({"contentTarget":"example://x","event":"launch","reason":"normal","appId":"org.rnwebos.example",)"
      R"("nid":"org.rnwebos.example","@system_native_app":true})");

  CHECK(launch.parameters == folly::dynamic::object("contentTarget", "example://x"));
  CHECK(launch.reason == "normal");
}

TEST_CASE("an app's own `event` and `reason` parameters survive when the app manager sent no launch event") {
  const auto launch = parseLaunchArgument(R"({"event":"goal","reason":"penalty","nid":"org.rnwebos.example"})");

  CHECK(launch.parameters == folly::dynamic::object("event", "goal")("reason", "penalty"));
}

TEST_CASE("no argument, a path, broken JSON and JSON that is not an object give empty parameters") {
  for (const std::string argument : {
           std::string(),
           std::string("/home/user/rn-webos-simulator/instances/platform"),
           std::string("{\"contentTarget\":"),
           std::string("[1,2]"),
           std::string("42"),
           "{\"a\":\"" + std::string(70 * 1024, 'a') + "\"}",
       }) {
    CAPTURE(argument.substr(0, 40));
    const auto launch = parseLaunchArgument(argument);
    CHECK(launch.parameters == folly::dynamic::object());
    CHECK(launch.reason.empty());
    CHECK_FALSE(launch.interfaceVersion.has_value());
  }
}

TEST_CASE("launchParametersOf reads the last argument, as the app manager passes it") {
  std::string program = "react-native-webos";
  std::string instance = "/instances/platform";
  std::string json = R"({"contentTarget":"example://x"})";

  std::array<char *, 1> alone{program.data()};
  CHECK(launchParametersOf(1, alone.data()).parameters == folly::dynamic::object());
  std::array<char *, 2> onTv{program.data(), json.data()};
  CHECK(launchParametersOf(2, onTv.data()).parameters["contentTarget"] == "example://x");
  std::array<char *, 2> inTheSimulator{program.data(), instance.data()};
  CHECK(launchParametersOf(2, inTheSimulator.data()).parameters == folly::dynamic::object());
  std::array<char *, 3> withBoth{program.data(), instance.data(), json.data()};
  CHECK(launchParametersOf(3, withBoth.data()).parameters["contentTarget"] == "example://x");
}

TEST_CASE("deepLinkOf is the contentTarget of the parameters, when it is a string worth delivering") {
  CHECK(deepLinkOf(folly::dynamic::object("contentTarget", "https://www.youtube.com/tv?v=9bZkp7q19f0")) ==
        "https://www.youtube.com/tv?v=9bZkp7q19f0");
  CHECK(deepLinkOf(folly::dynamic::object("contentTarget", "tt0111161")) == "tt0111161");
  CHECK_FALSE(deepLinkOf(folly::dynamic::object()).has_value());
  CHECK_FALSE(deepLinkOf(folly::dynamic::object("contentTarget", "")).has_value());
  CHECK_FALSE(deepLinkOf(folly::dynamic::object("contentTarget", 42)).has_value());
  CHECK_FALSE(deepLinkOf(folly::dynamic::object("contentTarget", std::string(9000, 'a'))).has_value());
  CHECK_FALSE(deepLinkOf(folly::dynamic::array("contentTarget")).has_value());
  CHECK_FALSE(deepLinkOf(nullptr).has_value());
}

} // namespace rnwebos
