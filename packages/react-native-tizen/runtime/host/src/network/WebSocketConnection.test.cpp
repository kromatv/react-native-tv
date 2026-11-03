#include "LoopbackServer.test.h"
#include "WebSocketConnection.h"
#include "WebSocketHandshake.h"

#include <doctest/doctest.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

class Events final {
 public:
  WebSocketListener listener() {
    return {
        .onOpen = [this](std::string protocol) { add("open " + protocol); },
        .onMessage = [this](std::string data, bool binary) { add((binary ? "binary " : "text ") + data); },
        .onClosed = [this](int code, std::string reason) { add("closed " + std::to_string(code) + " " + reason); },
        .onFailed = [this](std::string message) { add("failed " + message); },
    };
  }

  std::vector<std::string> waitFor(std::size_t count) {
    std::unique_lock lock(mutex_);
    changed_.wait_for(lock, 10s, [this, count] { return events_.size() >= count; });
    return events_;
  }

 private:
  void add(std::string event) {
    std::lock_guard lock(mutex_);
    events_.push_back(std::move(event));
    changed_.notify_all();
  }

  std::mutex mutex_;
  std::condition_variable changed_;
  std::vector<std::string> events_;
};

std::string headerValue(std::string_view request, std::string_view name) {
  const auto at = request.find(name);
  if (at == std::string_view::npos) return {};
  const auto start = at + name.size();
  return std::string(request.substr(start, request.find("\r\n", start) - start));
}

std::string readClientPayload(int socket) {
  const auto header = testing::readBytes(socket, 2);
  const auto length = static_cast<std::uint8_t>(header[1]) & 0x7f;
  const auto mask = testing::readBytes(socket, 4);
  auto payload = testing::readBytes(socket, length);
  for (std::size_t index = 0; index < payload.size(); ++index) payload[index] ^= mask[index % 4];
  return payload;
}

std::string serverFrame(std::uint8_t first, std::string_view payload) {
  return std::string{static_cast<char>(first), static_cast<char>(payload.size())} + std::string(payload);
}

void acceptHandshake(int socket, std::string_view extraHeaders = "") {
  const auto request = testing::readUntil(socket, "\r\n\r\n");
  const auto key = headerValue(request, "Sec-WebSocket-Key: ");
  testing::writeAll(
      socket,
      "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
          webSocketAccept(key) + "\r\n" + std::string(extraHeaders) + "\r\n");
}

} // namespace

TEST_CASE("a connection opens, echoes text and fragments, answers a ping and closes when the server does") {
  LoopbackServer server;
  Events events;
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

TEST_CASE("a close the client starts ends with the server's Close") {
  LoopbackServer server;
  Events events;
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

TEST_CASE("a connection fails when the server drops it, breaks the protocol or refuses the upgrade") {
  LoopbackServer server;
  Events dropped;
  Events masked;
  Events refused;
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
  Events events;

  WebSocketConnection connection(
      {.url = server.url("ws"), .protocols = {}, .headers = {{"X-Id", "1\r\nCookie: stolen"}}}, events.listener());
  const auto seen = events.waitFor(1);

  CHECK(seen == std::vector<std::string>{"failed The header X-Id holds a character HTTP forbids"});
  CHECK(server.accept(100ms) == -1);
}

TEST_CASE("destroying a connection says nothing more and tells the server it is going away") {
  LoopbackServer server;
  Events events;
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

} // namespace rntizen
