#include "ControlLink.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <folly/json.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>

namespace rntizen {

namespace {

class Listener {
 public:
  Listener() : descriptor_(socket(AF_INET, SOCK_STREAM, 0)) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    REQUIRE(bind(descriptor_, reinterpret_cast<sockaddr *>(&address), sizeof address) == 0);
    REQUIRE(listen(descriptor_, 4) == 0);
    socklen_t length = sizeof address;
    getsockname(descriptor_, reinterpret_cast<sockaddr *>(&address), &length);
    port_ = ntohs(address.sin_port);
  }
  ~Listener() {
    close(descriptor_);
  }
  Listener(const Listener &) = delete;
  Listener &operator=(const Listener &) = delete;

  std::uint16_t port() const {
    return port_;
  }
  int accept() const {
    return ::accept(descriptor_, nullptr, nullptr);
  }

 private:
  int descriptor_;
  std::uint16_t port_{0};
};

std::string readLine(int socket) {
  std::string line;
  char character = 0;
  while (recv(socket, &character, 1, 0) == 1 && character != '\n') line += character;
  return line;
}

void sendText(int socket, const std::string &text) {
  send(socket, text.data(), text.size(), MSG_NOSIGNAL);
}

bool waitFor(const std::function<bool()> &condition) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!condition()) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    ecore_main_loop_iterate();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return true;
}

} // namespace

TEST_CASE("ControlLink greets the listener and takes perf requests until they stop or the link drops") {
  ecore_init();
  const Listener listener;
  {
    const ControlLink link("127.0.0.1", listener.port(), "org.example.tv");
    const int client = listener.accept();
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

    CHECK(hello == folly::dynamic::object("type", "control")("version", 1)("app", "org.example.tv"));
    CHECK(turnedOn);
    CHECK(turnedOff);
    CHECK(turnedOnAgain);
    CHECK(dropped);
  }
  ecore_shutdown();
}

TEST_CASE("ControlLink answers a screenshot it cannot take with an error") {
  ecore_init();
  const Listener listener;
  {
    const ControlLink link("127.0.0.1", listener.port(), "org.example.tv");
    const int client = listener.accept();
    REQUIRE(client >= 0);
    readLine(client);

    sendText(client, "{\"type\":\"screenshot\",\"id\":7}\n");
    std::string answer;
    std::atomic<bool> answered{false};
    std::thread reader([client, &answer, &answered] {
      answer = readLine(client);
      answered = true;
    });
    waitFor([&answered] { return answered.load(); });
    reader.join();
    close(client);

    const auto reply = folly::parseJson(answer);
    CHECK(reply["type"] == "screenshot-error");
    CHECK(reply["id"] == 7);
  }
  ecore_shutdown();
}

TEST_CASE("ControlLink drops a listener whose request grows past 4 KiB") {
  const Listener listener;
  {
    const ControlLink link("127.0.0.1", listener.port(), "org.example.tv");
    const int client = listener.accept();
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
