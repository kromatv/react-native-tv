#pragma once

#include "NetworkingModule.h"
#include "TurboModuleTest.test.h"

#include "../network/LoopbackServer.test.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <string>

namespace rntizen::testing {

using facebook::jsi::String;

struct Request {
  std::string head;
  std::string body;
};

inline Request readRequest(int socket) {
  Request request{.head = testing::readUntil(socket, "\r\n\r\n"), .body = {}};
  if (const auto at = request.head.find("Content-Length: "); at != std::string::npos) {
    request.body = testing::readBytes(socket, std::stoul(request.head.substr(at + 16)));
  }
  return request;
}

inline std::future<Request> serveOnce(LoopbackServer &server, std::string response) {
  return std::async(std::launch::async, [&server, response = std::move(response)] {
    const int socket = server.accept(std::chrono::seconds(5));
    if (socket < 0) return Request{};
    auto request = readRequest(socket);
    testing::writeAll(socket, response);
    ::close(socket);
    return request;
  });
}

inline std::string headerOf(const std::string &head, const std::string &name) {
  const auto at = head.find(name + ": ");
  if (at == std::string::npos) return {};
  const auto start = at + name.size() + 2;
  return head.substr(start, head.find("\r\n", start) - start);
}

constexpr auto kHello = "HTTP/1.1 200 OK\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 5\r\nX-Multi: a\r\nX-Multi: b\r\n\r\nhello";
constexpr auto kOk = "HTTP/1.1 201 Created\r\nContent-Length: 2\r\n\r\nok";

struct Network {
  Network() {
    ModuleContext context{};
    context.emitDeviceEvent = events.emitter();
    module = std::make_shared<NetworkingModule>(invoker, context);
  }

  void send(
      const std::string &method,
      const std::string &url,
      int id,
      const std::string &headers,
      const std::string &data,
      const std::string &responseType = "text",
      bool incremental = false,
      double timeout = 5000) {
    auto &rt = *runtime;
    testing::call(
        rt,
        *module,
        "sendRequest",
        String::createFromUtf8(rt, method),
        String::createFromUtf8(rt, url),
        id,
        testing::evaluate(rt, headers),
        testing::evaluate(rt, data),
        String::createFromUtf8(rt, responseType),
        incremental,
        timeout,
        false);
  }

  /// The error didCompleteNetworkResponse reported for a request that never reached a server.
  std::string refusal(const std::string &headers, const std::string &data) {
    send("POST", server.url("http"), 9, headers, data);
    const auto completed = events.first("didCompleteNetworkResponse");
    CHECK(completed[0] == 9);
    CHECK(completed[2] == false);
    return completed[1].asString();
  }

  std::unique_ptr<facebook::jsi::Runtime> runtime = testing::makeRuntime();
  std::shared_ptr<testing::QueuedInvoker> invoker = std::make_shared<testing::QueuedInvoker>();
  testing::DeviceEvents events;
  LoopbackServer server;
  std::shared_ptr<NetworkingModule> module;
};

struct TemporaryFile {
  explicit TemporaryFile(const std::string &bytes)
      : path(std::filesystem::temp_directory_path() / ("rn-tizen-upload-" + std::to_string(::getpid()) + ".bin")) {
    std::ofstream(path, std::ios::binary) << bytes;
  }
  ~TemporaryFile() {
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
  }
  TemporaryFile(const TemporaryFile &) = delete;
  TemporaryFile &operator=(const TemporaryFile &) = delete;

  std::string uri() const {
    return "file://" + path.string();
  }

  std::filesystem::path path;
};

} // namespace rntizen::testing
