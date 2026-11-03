#include "DevServer.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

namespace {

constexpr const char *kBundleUrl =
    "http://192.168.1.20:8081/.expo/.virtual-metro-entry.bundle?platform=tizen&dev=true&app=org.example.app#top";

} // namespace

TEST_CASE("devServerOf reads the server, the bundle path and the app from a bundle URL") {
  const auto server = devServerOf(kBundleUrl);

  REQUIRE(server.has_value());
  CHECK(server->scheme == "http");
  CHECK(server->host == "192.168.1.20");
  CHECK(server->port == 8081);
  CHECK(server->bundlePath == ".expo/.virtual-metro-entry.bundle");
  CHECK(server->app == "org.example.app");
}

TEST_CASE("devServerOf takes the scheme's port when the URL names none") {
  CHECK(devServerOf("http://metro.local/index.bundle")->port == 80);
  CHECK(devServerOf("https://metro.local/index.bundle")->port == 443);
  CHECK(devServerOf("http://metro.local")->bundlePath.empty());
}

TEST_CASE("devServerOf keeps an IPv6 address in brackets, with or without a port") {
  const auto withPort = devServerOf("http://[fe80::1]:8081/index.bundle?app=a");
  const auto without = devServerOf("http://[fe80::1]/index.bundle");

  REQUIRE(withPort.has_value());
  CHECK(withPort->host == "[fe80::1]");
  CHECK(withPort->port == 8081);
  REQUIRE(without.has_value());
  CHECK(without->host == "[fe80::1]");
  CHECK(without->port == 80);
}

TEST_CASE("devServerOf refuses what is not an http or https server") {
  CHECK_FALSE(devServerOf("file:///opt/usr/apps/app/res/index.tizen.hbc").has_value());
  CHECK_FALSE(devServerOf("ws://192.168.1.20:8081/").has_value());
  CHECK_FALSE(devServerOf("192.168.1.20:8081/index.bundle").has_value());
  CHECK_FALSE(devServerOf("http://:8081/index.bundle").has_value());
  CHECK_FALSE(devServerOf("http://host:0/index.bundle").has_value());
  CHECK_FALSE(devServerOf("http://host:80a/index.bundle").has_value());
  CHECK_FALSE(devServerOf("http://[fe80::1]x/index.bundle").has_value());
}

TEST_CASE("devServerOf finds the app among the query's parameters, by its whole name") {
  CHECK(devServerOf("http://h:1/i.bundle?apps=x&app=org.example&dev=true")->app == "org.example");
  CHECK(devServerOf("http://h:1/i.bundle?platform=tizen")->app.empty());
  CHECK(devServerOf("http://h:1/i.bundle?app")->app.empty());
}

TEST_CASE("A dev server gives its HTTP and WebSocket URLs, secure when it is") {
  const auto server = devServerOf(kBundleUrl);
  const auto secure = devServerOf("https://tunnel.example:443/index.bundle");

  CHECK(server->authority() == "192.168.1.20:8081");
  CHECK(server->httpUrl("/status") == "http://192.168.1.20:8081/status");
  CHECK(server->socketUrl("/hot") == "ws://192.168.1.20:8081/hot");
  CHECK(secure->socketUrl("/hot") == "wss://tunnel.example:443/hot");
}

TEST_CASE("The packager socket names the TV, the app and the client, as Android does") {
  const auto server = devServerOf(kBundleUrl);

  CHECK(
      server->packagerUrl("QE55Q80 - Tizen 9.0") ==
      "ws://192.168.1.20:8081/message?device=QE55Q80%20-%20Tizen%209.0&app=org.example.app&clientid=DevSupport");
}

TEST_CASE("The inspector's device socket names the TV, the app and its device id, as Android does") {
  const auto server = devServerOf(kBundleUrl);

  CHECK(
      server->inspectorUrl("QE55Q80", "ab12") ==
      "ws://192.168.1.20:8081/inspector/device?name=QE55Q80&app=org.example.app&device=ab12&profiling=false");
}

TEST_CASE("HMRClient.setup gets the platform, the bundle path, the server and the Fast Refresh setting") {
  const auto setup = devServerOf(kBundleUrl)->hmrClientSetup(false);

  REQUIRE(setup.size() == 6);
  CHECK(setup[0] == "tizen");
  CHECK(setup[1] == ".expo/.virtual-metro-entry.bundle");
  CHECK(setup[2] == "192.168.1.20");
  CHECK(setup[3] == 8081);
  CHECK(setup[4] == false);
  CHECK(setup[5] == "http");
}

TEST_CASE("friendlyDeviceName gives the model and the Tizen version") {
  const DeviceProfile device{
      .modelName = "QE55Q80",
      .platformVersion = "9.0",
      .buildString = "T-PTMAKUC",
      .screenWidth = 1920,
      .screenHeight = 1080,
  };

  CHECK(friendlyDeviceName(device) == "QE55Q80 - Tizen 9.0");
}

TEST_CASE("encodeQueryValue keeps the unreserved characters and percent-encodes the rest") {
  CHECK(encodeQueryValue("aZ09-_.~") == "aZ09-_.~");
  CHECK(encodeQueryValue("a b/c&d=é") == "a%20b%2Fc%26d%3D%C3%A9");
}

} // namespace rntizen
