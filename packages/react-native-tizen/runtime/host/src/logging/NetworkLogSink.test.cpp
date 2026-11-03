#include "NetworkLogSink.h"

#include "../network/LoopbackServer.test.h"

#include <doctest/doctest.h>
#include <folly/json.h>

#include <chrono>
#include <string>
#include <thread>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

std::pair<std::string, std::string> readEntry(int socket) {
  const auto entry = folly::parseJson(testing::readUntil(socket, "\n"));
  return {entry["level"].asString(), entry["message"].asString()};
}

} // namespace

TEST_CASE("log lines reach the listener as one JSON object per line, in order") {
  LoopbackServer server;
  NetworkLogSink sink("127.0.0.1", server.port());

  sink.write(LogLevel::Info, "first");
  sink.write(LogLevel::Error, "second \"quoted\"");
  const int peer = server.accept(5s);
  const auto first = readEntry(peer);
  const auto second = readEntry(peer);
  ::close(peer);

  CHECK(first == std::pair<std::string, std::string>{"info", "first"});
  CHECK(second == std::pair<std::string, std::string>{"error", "second \"quoted\""});
}

TEST_CASE("a listener that comes late reads how many lines nobody heard, then the kept ones") {
  const auto port = LoopbackServer().port();
  NetworkLogSink sink("127.0.0.1", port);

  sink.write(LogLevel::Info, "kept");
  std::this_thread::sleep_for(100ms);
  sink.write(LogLevel::Debug, "dropped");
  LoopbackServer server(port);
  const int peer = server.accept(5s);
  const auto notice = readEntry(peer);
  const auto kept = readEntry(peer);
  ::close(peer);

  CHECK(notice.second == "1 earlier log lines were dropped while nothing listened");
  CHECK(kept == std::pair<std::string, std::string>{"info", "kept"});
}

} // namespace rntizen
