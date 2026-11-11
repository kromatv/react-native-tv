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
using testing::serverFrame;

constexpr std::size_t kLargeFrameHeader = 14;
constexpr std::size_t kCloseFrame = 8;

} // namespace

TEST_CASE("a close the client starts ends with the server's Close") {
  LoopbackServer server;
  WebSocketEvents events;
  std::string closePayload;
  std::thread peer([&server, &closePayload] {
    const int socket = server.accept(5s);
    acceptHandshake(socket);
    closePayload = readClientPayload(socket);
    testing::writeAll(socket, serverFrame(0x88, closePayload));
    ::close(socket);
  });

  WebSocketConnection connection({.url = server.url("ws"), .protocols = {}, .headers = {}}, events.listener());
  connection.close(1000, "bye");
  const auto seen = events.waitFor(2);
  peer.join();

  CHECK(seen == std::vector<std::string>{"open ", "closed 1000 bye"});
  CHECK(closePayload == std::string("\x03\xe8" "bye", 5));
}

TEST_CASE("destroying a connection says nothing more and tells the server it is going away") {
  LoopbackServer server;
  WebSocketEvents events;
  std::string goingAway;
  std::thread peer([&server, &goingAway] {
    const int socket = server.accept(5s);
    acceptHandshake(socket);
    goingAway = readClientPayload(socket);
    ::close(socket);
  });

  {
    WebSocketConnection connection({.url = server.url("ws"), .protocols = {}, .headers = {}}, events.listener());
    events.waitFor(1);
  }
  peer.join();

  CHECK(goingAway == std::string("\x03\xe9", 2));
  CHECK(events.waitFor(1) == std::vector<std::string>{"open "});
}

TEST_CASE("destroying a connection that waits for the handshake reports nothing") {
  LoopbackServer server;
  WebSocketEvents events;
  std::promise<void> asked;
  auto requestRead = asked.get_future();
  std::thread peer([&server, &asked] {
    const int socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
    asked.set_value();
    readUntilClosed(socket);
    ::close(socket);
  });

  {
    const WebSocketConnection connection(requestTo(server), events.listener());
    requestRead.wait_for(5s);
  }
  peer.join();

  CHECK(events.seen().empty());
}

TEST_CASE("a connection answers a Close without a status code with an empty Close") {
  LoopbackServer server;
  WebSocketEvents events;
  std::string answer = "unread";
  std::thread peer([&server, &answer] {
    const int socket = server.accept(5s);
    acceptHandshake(socket);
    testing::writeAll(socket, serverFrame(0x88, ""));
    answer = readClientPayload(socket);
    ::close(socket);
  });

  const WebSocketConnection connection(requestTo(server), events.listener());
  const auto seen = events.waitFor(2);
  peer.join();

  CHECK(seen == std::vector<std::string>{"open ", "closed 1005 "});
  CHECK(answer.empty());
}

TEST_CASE("a close the client starts ends when the server hangs up, with nothing sent after the Close") {
  LoopbackServer server;
  WebSocketEvents events;
  std::size_t afterClose = 1;
  std::thread peer([&server, &afterClose] {
    const int socket = server.accept(5s);
    acceptHandshake(socket);
    readClientPayload(socket);
    testing::writeAll(socket, serverFrame(0x89, "ping"));
    ::shutdown(socket, SHUT_WR);
    afterClose = readUntilClosed(socket);
    ::close(socket);
  });

  WebSocketConnection connection(requestTo(server), events.listener());
  connection.close(1000, "bye");
  connection.send(WebSocketOpcode::Text, "late");
  const auto seen = events.waitFor(2);
  peer.join();

  CHECK(seen == std::vector<std::string>{"open ", "closed 1000 bye"});
  CHECK(afterClose == 0);
}

TEST_CASE("destroying a connection that is closing sends no second Close") {
  LoopbackServer server;
  WebSocketEvents events;
  std::promise<void> closeRead;
  auto closed = closeRead.get_future();
  std::size_t afterClose = 1;
  std::thread peer([&server, &closeRead, &afterClose] {
    const int socket = server.accept(5s);
    acceptHandshake(socket);
    readClientPayload(socket);
    closeRead.set_value();
    afterClose = readUntilClosed(socket);
    ::close(socket);
  });

  {
    WebSocketConnection connection(requestTo(server), events.listener());
    connection.close(1000, "");
    closed.wait_for(5s);
  }
  peer.join();

  CHECK(events.seen() == std::vector<std::string>{"open "});
  CHECK(afterClose == 0);
}

TEST_CASE("a connection finishes a message the socket cannot take at once before its answer to a Close") {
  LoopbackServer server;
  WebSocketEvents events;
  std::promise<void> queued;
  auto messageQueued = queued.get_future();
  std::size_t received = 0;
  std::thread peer([&server, &messageQueued, &received] {
    const int socket = server.accept(5s);
    messageQueued.wait_for(5s);
    acceptHandshake(socket);
    testing::writeAll(socket, serverFrame(0x88, std::string("\x03\xe8", 2)));
    received = readUntilClosed(socket);
    ::close(socket);
  });

  WebSocketConnection connection(requestTo(server), events.listener());
  connection.send(WebSocketOpcode::Binary, std::string(kLargeMessage, 'x'));
  queued.set_value();
  const auto seen = events.waitFor(2);
  peer.join();

  CHECK(seen == std::vector<std::string>{"open ", "closed 1000 "});
  CHECK(received == kLargeMessage + kLargeFrameHeader + kCloseFrame);
}

} // namespace rntizen
