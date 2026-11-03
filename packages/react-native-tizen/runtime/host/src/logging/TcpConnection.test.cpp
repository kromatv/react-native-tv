#include "TcpConnection.h"

#include "../network/LoopbackServer.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

bool never() {
  return false;
}

} // namespace

TEST_CASE("connectTcp returns a blocking socket that sendAll writes through") {
  LoopbackServer server;

  const int socket = connectTcp("127.0.0.1", server.port(), 2s, never);
  const int peer = server.accept(2s);

  REQUIRE(socket >= 0);
  REQUIRE(peer >= 0);
  CHECK(sendAll(socket, "hello", 5));
  CHECK(testing::readBytes(peer, 5) == "hello");
  CHECK_FALSE(peerHasClosed(socket));
  ::close(peer);
  std::this_thread::sleep_for(50ms);
  CHECK(peerHasClosed(socket));
  ::close(socket);
}

TEST_CASE("connectTcp returns -1 for a host that is not an IPv4 address or a port nobody listens on") {
  const auto closedPort = LoopbackServer().port();

  CHECK(connectTcp("localhost", closedPort, 1s, never) == -1);
  CHECK(connectTcp("127.0.0.1", closedPort, 1s, never) == -1);
}

TEST_CASE("connectTcp gives up as soon as it is cancelled") {
  const auto started = std::chrono::steady_clock::now();

  const int socket = connectTcp("10.255.255.1", 9, 10s, [] { return true; });

  CHECK(socket == -1);
  CHECK(std::chrono::steady_clock::now() - started < 2s);
}

} // namespace rntizen
