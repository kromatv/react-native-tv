#include "HttpEngine.h"
#include "LoopbackServer.test.h"

#include <doctest/doctest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

struct Outcome {
  long status{0};
  std::string url;
  folly::dynamic headers;
  std::string body;
  std::string error;
};

std::future<Outcome> send(HttpEngine &engine, HttpRequest request) {
  auto promise = std::make_shared<std::promise<Outcome>>();
  auto outcome = std::make_shared<Outcome>();
  engine.start(
      std::move(request),
      HttpResponseHandler{
          .onResponse =
              [outcome](long status, folly::dynamic headers, std::string url) {
                outcome->status = status;
                outcome->headers = std::move(headers);
                outcome->url = std::move(url);
              },
          .onBody = [outcome](std::string body) { outcome->body = std::move(body); },
          .onComplete =
              [outcome, promise](std::string error, bool) {
                outcome->error = std::move(error);
                promise->set_value(*outcome);
              },
      });
  return promise->get_future();
}

HttpRequest request(const LoopbackServer &server, std::string method, std::optional<std::string> body) {
  return HttpRequest{
      .id = 1,
      .method = std::move(method),
      .url = server.url("http", "/path?q=1"),
      .headers = {{"X-Test", "yes"}},
      .body = std::move(body),
      .timeout = 5s,
  };
}

std::string serveOnce(const LoopbackServer &server, std::string_view response) {
  const int socket = server.accept(5s);
  if (socket < 0) return {};
  auto head = testing::readUntil(socket, "\r\n\r\n");
  const auto length = head.find("Content-Length: ");
  if (length != std::string::npos) head += testing::readBytes(socket, std::stoul(head.substr(length + 16)));
  testing::writeAll(socket, response);
  ::close(socket);
  return head;
}

constexpr std::string_view kOk = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nX-Multi: a\r\nX-Multi: b\r\n\r\nok";

} // namespace

TEST_CASE("a GET reaches the server without the body JavaScript gave it") {
  LoopbackServer server;
  HttpEngine engine;
  std::string seen;
  std::thread peer([&server, &seen] { seen = serveOnce(server, kOk); });

  const auto outcome = send(engine, request(server, "GET", std::string("ignored"))).get();
  peer.join();

  CHECK(seen.starts_with("GET /path?q=1 HTTP/1.1\r\n"));
  CHECK(seen.find("Content-Length") == std::string::npos);
  CHECK(seen.find("ignored") == std::string::npos);
  CHECK(seen.find("X-Test: yes\r\n") != std::string::npos);
  CHECK(seen.find("User-Agent: react-native-tizen\r\n") != std::string::npos);
  CHECK(outcome.status == 200);
  CHECK(outcome.body == "ok");
  CHECK(outcome.error.empty());
  CHECK(outcome.headers["X-Multi"].asString() == "a, b");
}

TEST_CASE("a PUT carries its body and a bodiless POST an empty one") {
  LoopbackServer server;
  HttpEngine engine;
  std::string put;
  std::string post;
  std::thread peer([&server, &put, &post] {
    put = serveOnce(server, kOk);
    post = serveOnce(server, kOk);
  });

  send(engine, request(server, "PUT", std::string(R"({"a":1})"))).get();
  send(engine, request(server, "POST", std::nullopt)).get();
  peer.join();

  CHECK(put.starts_with("PUT /path?q=1 HTTP/1.1\r\n"));
  CHECK(put.ends_with("\r\n\r\n{\"a\":1}"));
  CHECK(post.starts_with("POST /path?q=1 HTTP/1.1\r\n"));
  CHECK(post.find("Content-Length: 0\r\n") != std::string::npos);
}

TEST_CASE("a request with a header that would start another line fails without being sent") {
  LoopbackServer server;
  HttpEngine engine;
  auto injected = request(server, "GET", std::nullopt);
  injected.headers = {{"X-Test", "yes\r\nCookie: stolen"}};

  const auto outcome = send(engine, injected).get();

  CHECK(outcome.error == "The header X-Test holds a character HTTP forbids");
  CHECK(outcome.status == 0);
  CHECK(server.accept(100ms) == -1);
}

TEST_CASE("a request to a closed port completes with libcurl's error") {
  HttpEngine engine;
  const auto port = LoopbackServer().port();

  const auto outcome = send(
                           engine,
                           HttpRequest{
                               .id = 2,
                               .method = "GET",
                               .url = "http://127.0.0.1:" + std::to_string(port) + "/",
                               .headers = {},
                               .body = std::nullopt,
                               .timeout = 5s,
                           })
                           .get();

  CHECK_FALSE(outcome.error.empty());
  CHECK(outcome.status == 0);
}

TEST_CASE("an engine destroyed with requests in flight stops without answering them") {
  LoopbackServer server;
  std::atomic<bool> answered{false};
  int socket = -1;
  {
    HttpEngine engine;
    engine.start(
        request(server, "GET", std::nullopt),
        HttpResponseHandler{
            .onResponse = [](long, folly::dynamic, std::string) {},
            .onBody = [](std::string) {},
            .onComplete = [&answered](std::string, bool) { answered = true; },
        });
    socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
  }
  ::close(socket);

  CHECK_FALSE(answered);
}

} // namespace rntizen
