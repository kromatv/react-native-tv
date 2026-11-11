#include "HttpGet.h"
#include "LoopbackServer.test.h"
#include "NetworkError.h"

#include <doctest/doctest.h>

#include <chrono>
#include <string>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

std::thread answerOnce(const LoopbackServer &server, std::string response) {
  return std::thread([&server, response = std::move(response)] {
    const int socket = server.accept(5s);
    testing::readUntil(socket, "\r\n\r\n");
    testing::writeAll(socket, response);
    ::close(socket);
  });
}

} // namespace

TEST_CASE("httpGet returns the body of a 200 answer") {
  LoopbackServer server;
  auto peer = answerOnce(server, "HTTP/1.1 200 OK\r\nContent-Length: 6\r\n\r\nbundle");

  const auto body = httpGet(server.url("http", "/index.bundle"));
  peer.join();

  CHECK(body == "bundle");
}

TEST_CASE("httpGet names the URL and the status of any other answer") {
  LoopbackServer server;
  const auto url = server.url("http", "/missing");
  auto peer = answerOnce(server, "HTTP/1.1 404 Not Found\r\nContent-Length: 4\r\n\r\ngone");

  CHECK_THROWS_WITH_AS(httpGet(url), (url + " answered 404:\ngone").c_str(), NetworkError);
  peer.join();
}

TEST_CASE("httpGet names a server it cannot reach") {
  const auto url = LoopbackServer().url("http");

  CHECK_THROWS_WITH_AS(httpGet(url), doctest::Contains(("Cannot reach " + url).c_str()), NetworkError);
}

} // namespace rntizen
