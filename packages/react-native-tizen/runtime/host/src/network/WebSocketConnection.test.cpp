#include "WebSocketConnection.h"

#include "WebSocketPeer.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;
using testing::WebSocketEvents;
using testing::acceptHandshake;
using testing::kLargeMessage;
using testing::readClientPayload;
using testing::readUntilClosed;
using testing::requestTo;
using testing::resetConnection;
using testing::serverFrame;

constexpr std::size_t kOverlongHandshake = 70 * 1024;

} // namespace

TEST_CASE("a connection opens, echoes text and fragments, answers a ping and closes when the server does") {
  LoopbackServer server;
  WebSocketEvents events;
  std::string pong;
  std::thread peer([&server, &pong] {
    const int socket = server.accept(5s);
    acceptHandshake(socket, "Sec-WebSocket-Protocol: chat\r\n");
    const auto text = readClientPayload(socket);
    testing::writeAll(socket, serverFrame(0x81, text));
    testing::writeAll(socket, serverFrame(0x02, "ab") + serverFrame(0x80, "cd"));
    testing::writeAll(socket, serverFrame(0x89, "ping"));
    pong = readClientPayload(socket);
    testing::writeAll(socket, serverFrame(0x88, std::string("\x0f\xa1" "done", 6)));
    readClientPayload(socket);
    ::close(socket);
  });

  WebSocketConnection connection({.url = server.url("ws"), .protocols = {"chat"}, .headers = {}}, events.listener());
  connection.send(WebSocketOpcode::Text, "hello");
  const auto seen = events.waitFor(4);
  peer.join();

  CHECK(seen == std::vector<std::string>{"open chat", "text hello", "binary abcd", "closed 4001 done"});
  CHECK(pong == "ping");
}

TEST_CASE("a connection fails when the server drops it, breaks the protocol or refuses the upgrade") {
  LoopbackServer server;
  WebSocketEvents dropped;
  WebSocketEvents masked;
  WebSocketEvents refused;
  std::thread peer([&server] {
    int socket = server.accept(5s);
    acceptHandshake(socket);
    ::close(socket);
    socket = server.accept(5s);
    acceptHandshake(socket);
    testing::writeAll(socket, std::string("\x81\x81\x00\x00\x00\x00x", 7));
    testing::readBytes(socket, 1);
    ::close(socket);
    socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
    testing::writeAll(socket, "HTTP/1.1 404 Not Found\r\n\r\n");
    ::close(socket);
  });

  WebSocketConnection first({.url = server.url("ws"), .protocols = {}, .headers = {}}, dropped.listener());
  const auto droppedSeen = dropped.waitFor(2);
  WebSocketConnection second({.url = server.url("ws"), .protocols = {}, .headers = {}}, masked.listener());
  const auto maskedSeen = masked.waitFor(2);
  WebSocketConnection third({.url = server.url("ws"), .protocols = {}, .headers = {}}, refused.listener());
  const auto refusedSeen = refused.waitFor(1);
  peer.join();

  CHECK(droppedSeen == std::vector<std::string>{"open ", "failed The server closed the connection"});
  CHECK(maskedSeen == std::vector<std::string>{"open ", "failed The server sent a masked frame"});
  CHECK(refusedSeen == std::vector<std::string>{"failed Expected HTTP 101 response but was '404 Not Found'"});
}

TEST_CASE("a connection with a header no HTTP line can carry fails before it connects") {
  LoopbackServer server;
  WebSocketEvents events;

  WebSocketConnection connection(
      {.url = server.url("ws"), .protocols = {}, .headers = {{"X-Id", "1\r\nCookie: stolen"}}}, events.listener());
  const auto seen = events.waitFor(1);

  CHECK(seen == std::vector<std::string>{"failed The header X-Id holds a character HTTP forbids"});
  CHECK(server.accept(100ms) == -1);
}

TEST_CASE("a connection fails when the server hangs up during the handshake or answers past 64 KiB") {
  LoopbackServer server;
  WebSocketEvents hungUp;
  WebSocketEvents overlong;
  std::thread peer([&server] {
    int socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
    ::close(socket);
    socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
    testing::writeAll(socket, "HTTP/1.1 101 Switching Protocols\r\n" + std::string(kOverlongHandshake, 'x'));
    readUntilClosed(socket);
    ::close(socket);
  });

  WebSocketConnection first(requestTo(server), hungUp.listener());
  const auto hungUpSeen = hungUp.waitFor(1);
  WebSocketConnection second(requestTo(server), overlong.listener());
  const auto overlongSeen = overlong.waitFor(1);
  peer.join();

  CHECK(hungUpSeen == std::vector<std::string>{"failed The server closed the connection during the handshake"});
  CHECK(overlongSeen == std::vector<std::string>{"failed The handshake response is too long"});
}

TEST_CASE("a connection joins a message sent in three fragments and ignores a pong") {
  LoopbackServer server;
  WebSocketEvents events;
  std::thread peer([&server] {
    const int socket = server.accept(5s);
    acceptHandshake(socket);
    testing::writeAll(socket, serverFrame(0x8a, "pong") + serverFrame(0x01, "a") + serverFrame(0x00, "b"));
    testing::writeAll(socket, serverFrame(0x80, "c") + serverFrame(0x88, std::string("\x03\xe8", 2)));
    readClientPayload(socket);
    ::close(socket);
  });

  const WebSocketConnection connection(requestTo(server), events.listener());
  const auto seen = events.waitFor(3);
  peer.join();

  CHECK(seen == std::vector<std::string>{"open ", "text abc", "closed 1000 "});
}

TEST_CASE("a connection fails on a message started inside another or continued without a start") {
  LoopbackServer server;
  WebSocketEvents nested;
  WebSocketEvents orphaned;
  std::thread peer([&server] {
    int socket = server.accept(5s);
    acceptHandshake(socket);
    testing::writeAll(socket, serverFrame(0x01, "a") + serverFrame(0x81, "b"));
    readUntilClosed(socket);
    ::close(socket);
    socket = server.accept(5s);
    acceptHandshake(socket);
    testing::writeAll(socket, serverFrame(0x80, "x"));
    readUntilClosed(socket);
    ::close(socket);
  });

  WebSocketConnection first(requestTo(server), nested.listener());
  const auto nestedSeen = nested.waitFor(2);
  WebSocketConnection second(requestTo(server), orphaned.listener());
  const auto orphanedSeen = orphaned.waitFor(2);
  peer.join();

  CHECK(nestedSeen == std::vector<std::string>{"open ", "failed The server started a message inside another"});
  CHECK(orphanedSeen == std::vector<std::string>{"open ", "failed The server continued a message it never started"});
}

TEST_CASE("a connection fails when the server resets it, whether it was reading or sending") {
  LoopbackServer server;
  WebSocketEvents reading;
  WebSocketEvents sending;
  std::promise<void> opened;
  std::promise<void> queued;
  auto firstOpened = opened.get_future();
  auto messageQueued = queued.get_future();
  std::thread peer([&server, &firstOpened, &messageQueued] {
    int socket = server.accept(5s);
    acceptHandshake(socket);
    firstOpened.wait_for(5s);
    resetConnection(socket);
    socket = server.accept(5s);
    messageQueued.wait_for(5s);
    acceptHandshake(socket);
    testing::readBytes(socket, 1);
    resetConnection(socket);
  });

  WebSocketConnection first(requestTo(server), reading.listener());
  reading.waitFor(1);
  opened.set_value();
  const auto readingSeen = reading.waitFor(2);
  WebSocketConnection second(requestTo(server), sending.listener());
  second.send(WebSocketOpcode::Binary, std::string(kLargeMessage, 'x'));
  queued.set_value();
  const auto sendingSeen = sending.waitFor(2);
  peer.join();

  REQUIRE(readingSeen.size() == 2);
  REQUIRE(sendingSeen.size() == 2);
  CHECK(readingSeen[1].starts_with("failed "));
  CHECK(sendingSeen[1].starts_with("failed "));
}

} // namespace rntizen
