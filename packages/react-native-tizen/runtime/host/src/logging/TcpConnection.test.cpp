#include "TcpConnection.h"

#include "../network/LoopbackServer.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

constexpr std::size_t kMostQueuedConnections = 32;

bool never() {
  return false;
}

void reset(int socket) {
  const linger abortive{.l_onoff = 1, .l_linger = 0};
  setsockopt(socket, SOL_SOCKET, SO_LINGER, &abortive, sizeof abortive);
  ::close(socket);
}

bool waitUntilClosed(int socket) {
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (!peerHasClosed(socket)) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    std::this_thread::sleep_for(1ms);
  }
  return true;
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

TEST_CASE("connectTcp returns -1 at once for an address no route reaches") {
  const auto started = std::chrono::steady_clock::now();

  const int socket = connectTcp("255.255.255.255", 9, 10s, never);

  CHECK(socket == -1);
  CHECK(std::chrono::steady_clock::now() - started < 1s);
}

TEST_CASE("connectTcp gives up after its timeout when the listener queues no more connections") {
  const LoopbackServer server;
  std::vector<int> queued;

  int socket = connectTcp("127.0.0.1", server.port(), 250ms, never);
  while (socket >= 0 && queued.size() < kMostQueuedConnections) {
    queued.push_back(socket);
    socket = connectTcp("127.0.0.1", server.port(), 250ms, never);
  }
  for (const int connection : queued) ::close(connection);

  CHECK(socket == -1);
  CHECK_FALSE(queued.empty());
}

TEST_CASE("peerHasClosed tells a peer that wrote from one that reset the connection") {
  LoopbackServer server;
  const int socket = connectTcp("127.0.0.1", server.port(), 2s, never);
  const int peer = server.accept(2s);
  REQUIRE(socket >= 0);
  REQUIRE(peer >= 0);

  testing::writeAll(peer, "x");
  std::this_thread::sleep_for(50ms);
  const bool afterWrite = peerHasClosed(socket);
  reset(peer);
  const bool afterReset = waitUntilClosed(socket);
  const bool sent = sendAll(socket, "late", 4);
  ::close(socket);

  CHECK_FALSE(afterWrite);
  CHECK(afterReset);
  CHECK_FALSE(sent);
}

} // namespace rntizen
