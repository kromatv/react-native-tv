#include "WebSocketModule.h"
#include "WebSocketSockets.h"

#include "../TurboModuleTest.test.h"
#include "../blob/BlobModule.h"
#include "../blob/BlobStore.h"

#include "../../network/LoopbackServer.test.h"
#include "../../network/WebSocketHandshake.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <string>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using facebook::jsi::String;
using testing::LoopbackServer;

struct Frame {
  std::uint8_t first{0};
  std::string payload;
};

Frame readClientFrame(int socket) {
  const auto header = testing::readBytes(socket, 2);
  if (header.size() < 2) return {};
  const auto length = static_cast<std::uint8_t>(header[1]) & 0x7f;
  const auto mask = testing::readBytes(socket, 4);
  auto payload = testing::readBytes(socket, length);
  for (std::size_t index = 0; index < payload.size(); ++index) payload[index] ^= mask[index % 4];
  return {.first = static_cast<std::uint8_t>(header[0]), .payload = payload};
}

std::string serverFrame(std::uint8_t first, std::string_view payload) {
  return std::string{static_cast<char>(first), static_cast<char>(payload.size())} + std::string(payload);
}

std::string headerOf(const std::string &request, const std::string &name) {
  const auto at = request.find(name + ": ");
  if (at == std::string::npos) return {};
  const auto start = at + name.size() + 2;
  return request.substr(start, request.find("\r\n", start) - start);
}

std::string acceptHandshake(int socket, std::string_view protocol) {
  auto request = testing::readUntil(socket, "\r\n\r\n");
  testing::writeAll(
      socket,
      "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: " +
          webSocketAccept(headerOf(request, "Sec-WebSocket-Key")) + "\r\n" +
          (protocol.empty() ? std::string() : "Sec-WebSocket-Protocol: " + std::string(protocol) + "\r\n") + "\r\n");
  return request;
}

struct Sockets {
  Sockets() {
    ModuleContext context{};
    context.emitDeviceEvent = events.emitter();
    module = std::make_shared<WebSocketModule>(invoker, context);
  }

  void connect(int id, const std::string &url, const std::string &protocols, const std::string &options) {
    auto &rt = *runtime;
    testing::call(
        rt, *module, "connect", String::createFromUtf8(rt, url), testing::evaluate(rt, protocols), testing::evaluate(rt, options), id);
  }

  void forgetClosedSockets() {
    for (int round = 0; round < 50 && invoker->run(*runtime) == 0; ++round) std::this_thread::sleep_for(10ms);
  }

  std::unique_ptr<facebook::jsi::Runtime> runtime = testing::makeRuntime();
  std::shared_ptr<testing::QueuedInvoker> invoker = std::make_shared<testing::QueuedInvoker>();
  testing::DeviceEvents events;
  LoopbackServer server;
  std::shared_ptr<WebSocketModule> module;
};

} // namespace

TEST_CASE_FIXTURE(Sockets, "a socket opens with the protocols and headers JavaScript gave, and messages go both ways") {
  auto &rt = *runtime;
  std::promise<std::string> handshake;
  std::promise<void> sent;
  auto peer = std::async(std::launch::async, [this, &handshake, &sent] {
    const int socket = server.accept(5s);
    handshake.set_value(acceptHandshake(socket, "chat"));
    std::vector<Frame> received{readClientFrame(socket), readClientFrame(socket), readClientFrame(socket)};
    testing::writeAll(socket, serverFrame(0x81, "hi") + serverFrame(0x82, "ab"));
    sent.get_future().wait_for(5s);
    testing::writeAll(socket, serverFrame(0x82, "cd"));
    received.push_back(readClientFrame(socket));
    testing::writeAll(socket, serverFrame(0x88, received.back().payload));
    ::close(socket);
    return received;
  });

  connect(
      21,
      server.url("ws", "/socket"),
      R"js([' chat ', 'bad,proto', '', 5, 'v2\t'])js",
      R"js(({headers: {'X-Token': 'abc', 'X-Number': 5}}))js");
  const auto request = handshake.get_future().get();
  CHECK(events.first("websocketOpen") == folly::dynamic::object("id", 21)("protocol", "chat"));
  testing::call(rt, *module, "send", String::createFromUtf8(rt, "hello"), 21);
  testing::call(rt, *module, "sendBinary", String::createFromUtf8(rt, "AAEC"), 21);
  testing::call(rt, *module, "ping", 21);
  const auto messages = events.waitFor(3);
  WebSocketSockets::shared().setDeliversBlobs(21, true);
  sent.set_value();
  const auto blob = events.waitFor(4);
  testing::call(rt, *module, "close", 1000, String::createFromUtf8(rt, "bye"), 21);
  const auto frames = peer.get();
  const auto closed = events.waitFor(5);
  forgetClosedSockets();

  CHECK(headerOf(request, "Sec-WebSocket-Protocol") == "chat,v2");
  CHECK(headerOf(request, "X-Token") == "abc");
  CHECK(headerOf(request, "X-Number").empty());
  REQUIRE(frames.size() == 4);
  CHECK(frames[0].first == 0x81);
  CHECK(frames[0].payload == "hello");
  CHECK(frames[1].first == 0x82);
  CHECK(frames[1].payload == std::string("\0\1\2", 3));
  CHECK(frames[2].first == 0x89);
  CHECK(frames[2].payload.empty());
  CHECK(frames[3].first == 0x88);
  CHECK(frames[3].payload == std::string("\x03\xe8" "bye", 5));
  REQUIRE(messages.size() >= 3);
  CHECK(messages[1].payload == folly::dynamic::object("id", 21)("type", "text")("data", "hi"));
  CHECK(messages[2].payload == folly::dynamic::object("id", 21)("type", "binary")("data", "YWI="));
  REQUIRE(blob.size() >= 4);
  CHECK(blob[3].payload["type"] == "blob");
  CHECK(blob[3].payload["data"]["offset"] == 0);
  CHECK(blob[3].payload["data"]["size"] == 2);
  CHECK(BlobStore::shared().resolve(blob[3].payload["data"]["blobId"].asString(), 0, std::nullopt) == "cd");
  REQUIRE(closed.size() >= 5);
  CHECK(closed[4].name == "websocketClosed");
  CHECK(closed[4].payload == folly::dynamic::object("id", 21)("code", 1000)("reason", "bye"));
  CHECK(WebSocketSockets::shared().find(21) == nullptr);
  CHECK_FALSE(WebSocketSockets::shared().deliversBlobs(21));
}

TEST_CASE_FIXTURE(Sockets, "a binary message JavaScript sends as base64 that is not base64 fails, and the socket stays open") {
  auto &rt = *runtime;
  auto peer = std::async(std::launch::async, [this] {
    const int socket = server.accept(5s);
    acceptHandshake(socket, "");
    const auto frame = readClientFrame(socket);
    testing::writeAll(socket, serverFrame(0x88, std::string("\x0f\xa1" "done", 6)));
    readClientFrame(socket);
    ::close(socket);
    return frame;
  });

  connect(22, server.url("ws"), "undefined", "({})");
  CHECK(events.first("websocketOpen") == folly::dynamic::object("id", 22)("protocol", ""));
  testing::call(rt, *module, "sendBinary", String::createFromUtf8(rt, "not base64!"), 22);
  CHECK(events.first("websocketFailed") == folly::dynamic::object("id", 22)("message", "invalid base64"));
  testing::call(rt, *module, "send", String::createFromUtf8(rt, "still open"), 22);
  CHECK(peer.get().payload == "still open");
  CHECK(events.first("websocketClosed") == folly::dynamic::object("id", 22)("code", 4001)("reason", "done"));
}

TEST_CASE_FIXTURE(Sockets, "a socket that cannot connect fails, and is forgotten") {
  connect(23, "ws://127.0.0.1:1/", "[]", "({headers: 3})");
  const auto failed = events.first("websocketFailed");
  forgetClosedSockets();

  REQUIRE(failed.isObject());
  CHECK(failed["id"] == 23);
  CHECK_FALSE(failed["message"].asString().empty());
  CHECK(WebSocketSockets::shared().find(23) == nullptr);
}

TEST_CASE_FIXTURE(Sockets, "sending on a socket that is not open answers as Android does, and closing one does nothing") {
  auto &rt = *runtime;
  testing::call(rt, *module, "send", String::createFromUtf8(rt, "x"), 24);
  testing::call(rt, *module, "sendBinary", String::createFromUtf8(rt, "AA=="), 24);
  testing::call(rt, *module, "ping", 24);
  testing::call(rt, *module, "close", 1000, String::createFromUtf8(rt, "bye"), 24);

  const auto seen = events.waitFor(6);
  REQUIRE(seen.size() == 6);
  for (std::size_t index = 0; index < seen.size(); index += 2) {
    CHECK(seen[index].name == "websocketFailed");
    CHECK(seen[index].payload == folly::dynamic::object("id", 24)("message", "client is null"));
    CHECK(seen[index + 1].name == "websocketClosed");
    CHECK(seen[index + 1].payload == folly::dynamic::object("id", 24)("code", 0)("reason", "client is null"));
  }
  CHECK(testing::call(rt, *module, "addListener", String::createFromUtf8(rt, "websocketOpen")).isUndefined());
  CHECK(testing::call(rt, *module, "removeListeners", 1).isUndefined());
}

TEST_CASE_FIXTURE(Sockets, "BlobModule sends a blob's bytes over an open socket") {
  auto &rt = *runtime;
  auto blobs = std::make_shared<BlobModule>(invoker, ModuleContext{});
  const auto blobId = BlobStore::shared().store("blob bytes");
  auto peer = std::async(std::launch::async, [this] {
    const int socket = server.accept(5s);
    acceptHandshake(socket, "");
    const auto frame = readClientFrame(socket);
    testing::writeAll(socket, serverFrame(0x88, std::string("\x03\xe8", 2)));
    readClientFrame(socket);
    ::close(socket);
    return frame;
  });

  connect(25, server.url("ws"), "[]", "({})");
  events.first("websocketOpen");
  testing::call(rt, *blobs, "sendOverSocket", testing::evaluate(rt, "({blobId: 'missing'})"), 25);
  testing::call(rt, *blobs, "sendOverSocket", testing::evaluate(rt, "({blobId: '" + blobId + "', offset: 5})"), 25);
  const auto frame = peer.get();

  CHECK(frame.first == 0x82);
  CHECK(frame.payload == "bytes");
  CHECK(events.first("websocketClosed") == folly::dynamic::object("id", 25)("code", 1000)("reason", ""));
  BlobStore::shared().remove(blobId);
}

TEST_CASE("the socket registry replaces a socket under the same id and forgets every one at once") {
  auto &sockets = WebSocketSockets::shared();
  sockets.add(31, nullptr);
  sockets.add(31, nullptr);
  sockets.setDeliversBlobs(31, true);
  sockets.setDeliversBlobs(32, true);
  CHECK(sockets.deliversBlobs(31));

  sockets.forget(31);
  CHECK_FALSE(sockets.deliversBlobs(31));
  sockets.forget(33);
  sockets.forgetAll();
  CHECK_FALSE(sockets.deliversBlobs(32));
}

TEST_CASE_FIXTURE(Sockets, "the module forgets every socket when it goes away") {
  WebSocketSockets::shared().setDeliversBlobs(41, true);
  module.reset();
  CHECK_FALSE(WebSocketSockets::shared().deliversBlobs(41));
}

} // namespace rntizen
