#include "ControlLink.h"

#include "../network/LoopbackServer.test.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <folly/json.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using testing::LoopbackServer;

constexpr auto kAcceptTimeout = 5s;
constexpr const char *kApp = "org.example.tv";

std::string readLine(int socket) {
  auto line = testing::readUntil(socket, "\n");
  if (line.ends_with('\n')) line.pop_back();
  return line;
}

void sendText(int socket, const std::string &text) {
  testing::writeAll(socket, text);
}

bool waitFor(const std::function<bool()> &condition) {
  const auto deadline = std::chrono::steady_clock::now() + 5s;
  while (!condition()) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    ecore_main_loop_iterate();
    std::this_thread::sleep_for(10ms);
  }
  return true;
}

std::string readOnMainLoop(const std::function<std::string()> &read) {
  std::string received;
  std::atomic<bool> done{false};
  std::thread reader([&read, &received, &done] {
    received = read();
    done = true;
  });
  waitFor([&done] { return done.load(); });
  reader.join();
  return received;
}

} // namespace

TEST_CASE("ControlLink greets the listener and takes perf requests until they stop or the link drops") {
  ecore_init();
  const LoopbackServer listener;
  {
    const ControlLink link("127.0.0.1", listener.port(), kApp);
    const int client = listener.accept(kAcceptTimeout);
    REQUIRE(client >= 0);

    const auto hello = folly::parseJson(readLine(client));
    sendText(client, "not json\n{\"type\":\"perf\",");
    sendText(client, "\"on\":true}\n{\"type\":\"screenshot\",\"id\":99999999999999999999}\n");
    const bool turnedOn = waitFor([&link] { return link.perfRequested(); });
    sendText(client, "[]\n{\"id\":1}\n{\"type\":5}\n{\"type\":\"other\"}\n{\"type\":\"screenshot\"}\n");
    sendText(client, "{\"type\":\"screenshot\",\"id\":\"7\"}\n{\"type\":\"screenshot\",\"id\":-1}\n");
    sendText(client, "{\"type\":\"perf\"}\n{\"type\":\"perf\",\"on\":1}\n{\"type\":\"perf\",\"on\":false}\n");
    const bool turnedOff = waitFor([&link] { return !link.perfRequested(); });
    sendText(client, "{\"type\":\"perf\",\"on\":true}\n");
    const bool turnedOnAgain = waitFor([&link] { return link.perfRequested(); });
    close(client);
    const bool dropped = waitFor([&link] { return !link.perfRequested(); });

    CHECK(hello == folly::dynamic::object("type", "control")("version", 1)("app", kApp));
    CHECK(turnedOn);
    CHECK(turnedOff);
    CHECK(turnedOnAgain);
    CHECK(dropped);
  }
  ecore_shutdown();
}

TEST_CASE("ControlLink answers a screenshot it cannot take with an error") {
  ecore_init();
  const LoopbackServer listener;
  {
    const ControlLink link("127.0.0.1", listener.port(), kApp);
    const int client = listener.accept(kAcceptTimeout);
    REQUIRE(client >= 0);
    readLine(client);

    sendText(client, "{\"type\":\"screenshot\",\"id\":7}\n");
    const auto answer = readOnMainLoop([client] { return readLine(client); });
    close(client);

    const auto reply = folly::parseJson(answer);
    CHECK(reply["type"] == "screenshot-error");
    CHECK(reply["id"] == 7);
  }
  ecore_shutdown();
}

TEST_CASE("ControlLink sends a screenshot as a header line that sizes the pixels, then the pixels") {
  ecore_init();
  const LoopbackServer listener;
  const std::vector<std::uint8_t> pixels{1, 2, 3, 4, 5, 6, 7, 8};
  {
    const ControlLink link("127.0.0.1", listener.port(), kApp, [&pixels](const CaptureDone &done) {
      done(ScreenCapture{.width = 2, .height = 1, .stride = 8, .pixels = pixels});
    });
    const int client = listener.accept(kAcceptTimeout);
    REQUIRE(client >= 0);
    readLine(client);

    sendText(client, "{\"type\":\"screenshot\",\"id\":3}\n");
    const auto answer =
        readOnMainLoop([client] { return testing::readUntil(client, "\n") + testing::readBytes(client, 8); });
    close(client);

    const auto newline = answer.find('\n');
    REQUIRE(newline != std::string::npos);
    CHECK(folly::parseJson(answer.substr(0, newline)) ==
          folly::dynamic::object("type", "screenshot")("id", 3)("width", 2)("height", 1)("stride", 8)("bytes", 8));
    CHECK(answer.substr(newline + 1) == std::string(pixels.begin(), pixels.end()));
  }
  ecore_shutdown();
}

TEST_CASE("ControlLink answers nothing more once it is destroyed while a screenshot is being taken") {
  ecore_init();
  const LoopbackServer listener;
  std::vector<CaptureDone> unfinished;
  int client = -1;
  {
    const ControlLink link("127.0.0.1", listener.port(), kApp, [&unfinished](CaptureDone done) {
      unfinished.push_back(std::move(done));
    });
    client = listener.accept(kAcceptTimeout);
    REQUIRE(client >= 0);
    readLine(client);

    sendText(client, "{\"type\":\"screenshot\",\"id\":4}\n");
    REQUIRE(waitFor([&unfinished] { return !unfinished.empty(); }));
  }
  char byte = 0;
  const auto received = recv(client, &byte, 1, 0);
  close(client);
  unfinished.clear();
  ecore_shutdown();

  CHECK(received <= 0);
}

TEST_CASE("ControlLink keeps calling until a listener starts") {
  const auto port = LoopbackServer().port();
  const ControlLink link("127.0.0.1", port, kApp);
  std::this_thread::sleep_for(50ms);
  const LoopbackServer listener(port);

  const int client = listener.accept(kAcceptTimeout);
  REQUIRE(client >= 0);
  const auto hello = folly::parseJson(readLine(client));
  close(client);

  CHECK(hello == folly::dynamic::object("type", "control")("version", 1)("app", kApp));
}

TEST_CASE("ControlLink drops a listener whose request grows past 4 KiB") {
  const LoopbackServer listener;
  {
    const ControlLink link("127.0.0.1", listener.port(), kApp);
    const int client = listener.accept(kAcceptTimeout);
    REQUIRE(client >= 0);
    readLine(client);

    sendText(client, std::string(5000, 'x'));
    char byte = 0;
    const auto received = recv(client, &byte, 1, 0);
    close(client);

    CHECK(received <= 0);
  }
}

} // namespace rntizen
