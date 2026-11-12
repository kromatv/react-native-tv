#include "DevelopmentLinks.h"

#include "../logging/Log.h"
#include "../network/LoopbackServer.test.h"

#include <doctest/doctest.h>
#include <folly/json.h>

#include <chrono>
#include <string>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

constexpr const char *kPackageId = "org.example.tv";

HostConfig configFor(const LoopbackServer &server, bool development) {
  HostConfig config{};
  config.packageId = kPackageId;
  config.logEndpoint = LogEndpoint{.host = "127.0.0.1", .port = server.port()};
  config.development = development;
  return config;
}

} // namespace

TEST_CASE("openDevelopmentLinks opens nothing without a log endpoint") {
  HostConfig config{};
  config.development = true;

  CHECK(openDevelopmentLinks(config) == nullptr);
}

TEST_CASE("openDevelopmentLinks streams a release build's logs and opens no control link") {
  const LoopbackServer server;

  const auto link = openDevelopmentLinks(configFor(server, false));
  log(LogLevel::Info, "streamed");
  const int peer = server.accept(5s);
  const auto hello = testing::readUntil(peer, "\n");
  const auto line = testing::readUntil(peer, "\n");
  setLogSink(nullptr);
  ::close(peer);

  CHECK(link == nullptr);
  CHECK(folly::parseJson(hello) == folly::dynamic::object("type", "log")("version", 1)("app", kPackageId));
  CHECK(folly::parseJson(line)["message"] == "streamed");
}

TEST_CASE("openDevelopmentLinks opens a development build's control link beside its log stream") {
  const LoopbackServer server;

  const auto link = openDevelopmentLinks(configFor(server, true));
  const int peer = server.accept(5s);
  const auto hello = testing::readUntil(peer, "\n");
  setLogSink(nullptr);
  ::close(peer);

  CHECK(link != nullptr);
  CHECK(folly::parseJson(hello) == folly::dynamic::object("type", "control")("version", 1)("app", kPackageId));
}

} // namespace rntizen
