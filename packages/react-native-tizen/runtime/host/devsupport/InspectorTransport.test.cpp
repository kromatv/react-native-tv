#include "InspectorTransport.h"

#include "network/WebSocketHandshake.h"

#include <doctest/doctest.h>

#include <Ecore.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace rntizen {

namespace {

using namespace std::chrono_literals;
using facebook::react::jsinspector_modern::IWebSocketDelegate;

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

bool readable(int fd, const std::atomic<bool> &stopping, std::chrono::milliseconds timeout) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (!stopping && std::chrono::steady_clock::now() < deadline) {
    pollfd poller{.fd = fd, .events = POLLIN, .revents = 0};
    if (::poll(&poller, 1, kPollMs) > 0) return true;
  }
  return false;
}

// The inspector proxy's end of `/inspector/device`, for one client: it answers
// the handshake, sends `greeting`, and keeps the first text the client sends.
class ProxyPeer final {
 public:
  ProxyPeer(std::uint16_t port, std::string greeting)
      : greeting_(std::move(greeting)), listener_(::socket(AF_INET, SOCK_STREAM, 0)) {
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

  ~ProxyPeer() {
    stopping_ = true;
    thread_.join();
    ::close(listener_);
  }

  ProxyPeer(const ProxyPeer &) = delete;
  ProxyPeer &operator=(const ProxyPeer &) = delete;

  std::optional<std::string> received() {
    std::lock_guard lock(mutex_);
    return received_;
  }

 private:
  void serve() {
    if (!readable(listener_, stopping_, 10s)) return;
    const int client = ::accept(listener_, nullptr, nullptr);
    if (client < 0) return;
    handshake(client);
    std::string frame{static_cast<char>(0x81), static_cast<char>(greeting_.size())};
    frame += greeting_;
    ::send(client, frame.data(), frame.size(), MSG_NOSIGNAL);
    if (readable(client, stopping_, 5s)) {
      unsigned char header[6];
      if (::recv(client, header, sizeof(header), MSG_WAITALL) == sizeof(header)) {
        std::string payload(header[1] & 0x7f, '\0');
        ::recv(client, payload.data(), payload.size(), MSG_WAITALL);
        for (std::size_t index = 0; index < payload.size(); ++index) {
          payload[index] ^= static_cast<char>(header[2 + index % 4]);
        }
        std::lock_guard lock(mutex_);
        received_ = std::move(payload);
      }
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

  const std::string greeting_;
  const int listener_;
  std::atomic<bool> stopping_{false};
  std::mutex mutex_;
  std::optional<std::string> received_;
  std::thread thread_;
};

class RecordingDelegate final : public IWebSocketDelegate {
 public:
  void didFailWithError(std::optional<int>, std::string error) override {
    failures.push_back(std::move(error));
  }
  void didReceiveMessage(std::string_view message) override {
    messages.emplace_back(message);
  }
  void didOpen() override {
    opened = true;
  }
  void didClose() override {
    closed = true;
  }

  std::vector<std::string> failures;
  std::vector<std::string> messages;
  bool opened{false};
  bool closed{false};
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

TEST_CASE("An inspector socket reports the proxy's messages on the main loop and sends the device's") {
  ecore_init();
  const auto port = freePort();
  ProxyPeer peer(port, R"({"event":"getPages"})");
  auto delegate = std::make_shared<RecordingDelegate>();
  InspectorTransport transport;

  const auto url = "ws://127.0.0.1:" + std::to_string(port) + "/inspector/device";
  const auto socket = transport.connectWebSocket(url, delegate);
  socket->send(R"({"event":"getPages","payload":[]})");

  REQUIRE(runLoopUntil([&] { return delegate->opened && !delegate->messages.empty(); }, 10s));
  CHECK(delegate->messages.front() == R"({"event":"getPages"})");
  REQUIRE(runLoopUntil([&] { return peer.received().has_value(); }, 10s));
  CHECK(*peer.received() == R"({"event":"getPages","payload":[]})");
  CHECK(runLoopUntil([&] { return delegate->closed || !delegate->failures.empty(); }, 10s));
}

TEST_CASE("An inspector socket with no proxy to reach reports the failure") {
  ecore_init();
  auto delegate = std::make_shared<RecordingDelegate>();
  InspectorTransport transport;

  const auto socket =
      transport.connectWebSocket("ws://127.0.0.1:" + std::to_string(freePort()) + "/inspector/device", delegate);

  CHECK(runLoopUntil([&] { return !delegate->failures.empty(); }, 10s));
  CHECK_FALSE(delegate->opened);
}

TEST_CASE("A scheduled callback runs on the main loop after its delay") {
  ecore_init();
  InspectorTransport transport;
  bool ran = false;
  const auto scheduled = std::chrono::steady_clock::now();

  transport.scheduleCallback([&ran] { ran = true; }, 50ms);

  CHECK(runLoopUntil([&] { return ran; }, 5s));
  CHECK(std::chrono::steady_clock::now() - scheduled >= 50ms);
}

} // namespace rntizen
