#include "PackagerConnection.h"

#include "network/WebSocketHandshake.h"

#include <doctest/doctest.h>

#include <Ecore.h>
#include <folly/json.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;

constexpr int kPollMs = 50;

std::uint16_t freePort() {
  const int probe = ::socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  ::bind(probe, reinterpret_cast<sockaddr *>(&address), sizeof(address));
  socklen_t length = sizeof(address);
  ::getsockname(probe, reinterpret_cast<sockaddr *>(&address), &length);
  ::close(probe);
  return ntohs(address.sin_port);
}

std::string packagerUrl(std::uint16_t port) {
  return "ws://127.0.0.1:" + std::to_string(port) + "/message";
}

bool readable(int fd, const std::atomic<bool> &stopping, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!stopping && std::chrono::steady_clock::now() < deadline) {
    pollfd poller{.fd = fd, .events = POLLIN, .revents = 0};
    if (::poll(&poller, 1, kPollMs) > 0) return true;
  }
  return false;
}

// The dev server's end of `/message`, for one client: it answers the
// handshake, sends `messages`, and keeps the first text the client sends back.
class PackagerPeer final {
 public:
  PackagerPeer(std::uint16_t port, std::vector<std::string> messages, bool waitsForReply)
      : messages_(std::move(messages)), waitsForReply_(waitsForReply), listener_(::socket(AF_INET, SOCK_STREAM, 0)) {
    const int reuse = 1;
    ::setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    REQUIRE(::bind(listener_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0);
    ::listen(listener_, 1);
    thread_ = std::thread([this] { serve(); });
  }

  ~PackagerPeer() {
    stopping_ = true;
    thread_.join();
    ::close(listener_);
  }

  PackagerPeer(const PackagerPeer &) = delete;
  PackagerPeer &operator=(const PackagerPeer &) = delete;

  std::optional<std::string> reply() {
    std::lock_guard lock(mutex_);
    return reply_;
  }

 private:
  void serve() {
    if (!readable(listener_, stopping_, 10s)) return;
    const int client = ::accept(listener_, nullptr, nullptr);
    if (client < 0) return;
    handshake(client);
    for (const auto &message : messages_) sendText(client, message);
    if (waitsForReply_ && readable(client, stopping_, 5s)) {
      auto text = readFrame(client);
      std::lock_guard lock(mutex_);
      reply_ = std::move(text);
    }
    ::close(client);
  }

  void handshake(int client) const {
    std::string request;
    char chunk[512];
    while (request.find("\r\n\r\n") == std::string::npos && readable(client, stopping_, 5s)) {
      const auto received = ::recv(client, chunk, sizeof(chunk), 0);
      if (received <= 0) return;
      request.append(chunk, static_cast<std::size_t>(received));
    }
    const std::string field = "Sec-WebSocket-Key: ";
    const auto start = request.find(field) + field.size();
    const auto key = request.substr(start, request.find("\r\n", start) - start);
    const auto answer = "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
                        "Sec-WebSocket-Accept: " +
        webSocketAccept(key) + "\r\n\r\n";
    ::send(client, answer.data(), answer.size(), MSG_NOSIGNAL);
  }

  static void sendText(int client, const std::string &text) {
    std::string frame{static_cast<char>(0x81), static_cast<char>(text.size())};
    frame += text;
    ::send(client, frame.data(), frame.size(), MSG_NOSIGNAL);
  }

  static std::string readFrame(int client) {
    unsigned char header[6];
    if (::recv(client, header, sizeof(header), MSG_WAITALL) != sizeof(header)) return {};
    std::string payload(header[1] & 0x7f, '\0');
    ::recv(client, payload.data(), payload.size(), MSG_WAITALL);
    for (std::size_t index = 0; index < payload.size(); ++index) {
      payload[index] ^= static_cast<char>(header[2 + index % 4]);
    }
    return payload;
  }

  const std::vector<std::string> messages_;
  const bool waitsForReply_;
  const int listener_;
  std::atomic<bool> stopping_{false};
  std::mutex mutex_;
  std::optional<std::string> reply_;
  std::thread thread_;
};

bool runLoopUntil(const std::function<bool()> &done, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!done()) {
    if (std::chrono::steady_clock::now() > deadline) return false;
    ecore_main_loop_iterate();
    std::this_thread::sleep_for(5ms);
  }
  return true;
}

} // namespace

TEST_CASE("The dev server's reload and devMenu reach their handlers on the main loop") {
  ecore_init();
  const auto port = freePort();
  PackagerPeer peer(port, {R"({"version":2,"method":"reload"})", R"({"version":2,"method":"devMenu"})"}, false);
  int reloads = 0;
  int menus = 0;

  PackagerConnection connection(
      packagerUrl(port), {.reload = [&reloads] { reloads += 1; }, .devMenu = [&menus] { menus += 1; }});

  CHECK(runLoopUntil([&] { return reloads == 1 && menus == 1; }, 10s));
}

TEST_CASE("A request the app has no handler for is answered with an error") {
  ecore_init();
  const auto port = freePort();
  PackagerPeer peer(port, {R"({"version":2,"id":3,"method":"captureHeap"})"}, true);

  PackagerConnection connection(packagerUrl(port), {.reload = [] {}, .devMenu = [] {}});

  REQUIRE(runLoopUntil([&] { return peer.reply().has_value(); }, 10s));
  const auto reply = folly::parseJson(*peer.reply());
  CHECK(reply["id"] == 3);
  CHECK(reply["error"] == "No request handler for method: captureHeap");
}

TEST_CASE("With no dev server yet, the socket tries again until one answers") {
  ecore_init();
  const auto port = freePort();
  int reloads = 0;
  PackagerConnection connection(packagerUrl(port), {.reload = [&reloads] { reloads += 1; }, .devMenu = [] {}});
  runLoopUntil([] { return false; }, 500ms);

  PackagerPeer peer(port, {R"({"version":2,"method":"reload"})"}, false);

  CHECK(runLoopUntil([&] { return reloads == 1; }, 10s));
}

} // namespace rntizen
