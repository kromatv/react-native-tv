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

constexpr const char *kApp = "org.example.tv";
constexpr int kUnheardLines = 300;

std::pair<std::string, std::string> readEntry(int socket) {
  const auto entry = folly::parseJson(testing::readUntil(socket, "\n"));
  return {entry["level"].asString(), entry["message"].asString()};
}

} // namespace

TEST_CASE("log lines reach the listener as one JSON object per line, in order") {
  LoopbackServer server;
  NetworkLogSink sink("127.0.0.1", server.port(), "org.example.tv");

  sink.write(LogLevel::Info, "first");
  sink.write(LogLevel::Error, "second \"quoted\"");
  const int peer = server.accept(5s);
  const auto hello = folly::parseJson(testing::readUntil(peer, "\n"));
  const auto first = readEntry(peer);
  const auto second = readEntry(peer);
  ::close(peer);

  CHECK(hello == folly::dynamic::object("type", "log")("version", 1)("app", "org.example.tv"));
  CHECK(first == std::pair<std::string, std::string>{"info", "first"});
  CHECK(second == std::pair<std::string, std::string>{"error", "second \"quoted\""});
}

TEST_CASE("a listener that comes late reads how many lines nobody heard, then the kept ones") {
  const auto port = LoopbackServer().port();
  NetworkLogSink sink("127.0.0.1", port, "org.example.tv");

  sink.write(LogLevel::Info, "kept");
  std::this_thread::sleep_for(100ms);
  sink.write(LogLevel::Debug, "dropped");
  LoopbackServer server(port);
  const int peer = server.accept(5s);
  testing::readUntil(peer, "\n");
  const auto notice = readEntry(peer);
  const auto kept = readEntry(peer);
  ::close(peer);

  CHECK(notice.second == "1 earlier log lines were dropped while nothing listened");
  CHECK(kept == std::pair<std::string, std::string>{"info", "kept"});
}

TEST_CASE("a listener that went away is noticed before the next line, which goes to the next listener") {
  LoopbackServer server;
  NetworkLogSink sink("127.0.0.1", server.port(), kApp);

  sink.write(LogLevel::Info, "first");
  const int gone = server.accept(5s);
  testing::readUntil(gone, "\n");
  const auto first = readEntry(gone);
  ::close(gone);
  std::this_thread::sleep_for(50ms);
  sink.write(LogLevel::Info, "second");
  const int next = server.accept(5s);
  const auto hello = folly::parseJson(testing::readUntil(next, "\n"));
  const auto second = readEntry(next);
  ::close(next);

  CHECK(first == std::pair<std::string, std::string>{"info", "first"});
  CHECK(hello["app"] == kApp);
  CHECK(second == std::pair<std::string, std::string>{"info", "second"});
}

TEST_CASE("a sink nothing hears keeps its last 200 lines and counts the rest") {
  const auto port = LoopbackServer().port();
  NetworkLogSink sink("127.0.0.1", port, kApp);

  sink.write(LogLevel::Info, "first");
  std::this_thread::sleep_for(100ms);
  for (int index = 0; index < kUnheardLines; ++index) sink.write(LogLevel::Info, "line " + std::to_string(index));
  sink.write(LogLevel::Debug, "dropped");
  LoopbackServer server(port);
  const int peer = server.accept(5s);
  testing::readUntil(peer, "\n");
  const auto notice = readEntry(peer);
  const auto oldestKept = readEntry(peer);
  ::close(peer);

  CHECK(notice.second == "102 earlier log lines were dropped while nothing listened");
  CHECK(oldestKept.second == "line 100");
}

} // namespace rntizen
