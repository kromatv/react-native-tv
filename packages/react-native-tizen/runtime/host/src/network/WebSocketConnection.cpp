#include "WebSocketConnection.h"

#include "WebSocketHandshake.h"
#include "WebSocketTransport.h"

#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <random>
#include <stdexcept>

namespace rntizen {

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::uint64_t kMaxMessageBytes = 64ULL * 1024 * 1024;
constexpr std::size_t kMaxHandshakeBytes = 64 * 1024;
constexpr std::size_t kReadChunk = 16 * 1024;
constexpr auto kHandshakeTimeout = std::chrono::seconds(10);
constexpr auto kCloseTimeout = std::chrono::seconds(5);
constexpr int kGoingAway = 1001;
constexpr int kNoStatus = 1005;

int pollTimeout(std::optional<Clock::time_point> deadline) {
  if (!deadline) return -1;
  const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - Clock::now()).count();
  return static_cast<int>(std::max<std::int64_t>(left, 0));
}

} // namespace

class WebSocketConnection::Session {
 public:
  Session(const std::string &url, const std::atomic<bool> &abort)
      : transport_(url, abort), random_(std::random_device{}()) {}

  void queueRaw(std::string bytes) {
    outbox_.push_back(std::move(bytes));
  }

  void queue(WebSocketOpcode opcode, std::string_view payload) {
    std::array<std::uint8_t, 4> mask{};
    for (auto &byte : mask) byte = static_cast<std::uint8_t>(random_() & 0xff);
    queueRaw(encodeWebSocketFrame(opcode, payload, mask));
  }

  void flush() {
    while (!outbox_.empty()) {
      const std::string_view front = outbox_.front();
      const auto sent = transport_.send(front.substr(sentOfFront_));
      if (sent == 0) return;
      sentOfFront_ += sent;
      if (sentOfFront_ == front.size()) {
        outbox_.pop_front();
        sentOfFront_ = 0;
      }
    }
  }

  bool read() {
    std::array<char, kReadChunk> chunk{};
    while (true) {
      const auto received = transport_.receive(chunk.data(), chunk.size());
      if (!received) return true;
      if (*received == 0) return false;
      inbox.append(chunk.data(), *received);
    }
  }

  void wait(int wakeFd, std::optional<Clock::time_point> deadline) const {
    std::array<pollfd, 2> fds{{
        {.fd = transport_.socket(),
         .events = static_cast<short>(POLLIN | (outbox_.empty() ? 0 : POLLOUT)),
         .revents = 0},
        {.fd = wakeFd, .events = POLLIN, .revents = 0},
    }};
    poll(fds.data(), fds.size(), pollTimeout(deadline));
    if ((fds[1].revents & POLLIN) != 0) {
      std::uint64_t count = 0;
      [[maybe_unused]] auto ignored = ::read(wakeFd, &count, sizeof(count));
    }
  }

  void drain(int wakeFd, Clock::time_point deadline) {
    flush();
    while (!outbox_.empty() && Clock::now() < deadline) {
      wait(wakeFd, deadline);
      flush();
    }
  }

  std::string inbox;
  std::optional<WebSocketOpcode> fragmentOpcode;
  std::string fragments;
  std::optional<std::pair<int, std::string>> closeSent;
  Clock::time_point closeDeadline;

 private:
  WebSocketTransport transport_;
  std::deque<std::string> outbox_;
  std::size_t sentOfFront_{0};
  std::mt19937 random_;
};

WebSocketConnection::WebSocketConnection(WebSocketRequest request, WebSocketListener listener)
    : request_(std::move(request)),
      listener_(std::move(listener)),
      wakeFd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
      thread_([this] { run(); }) {}

WebSocketConnection::~WebSocketConnection() {
  stopping_ = true;
  wake();
  thread_.join();
  ::close(wakeFd_);
}

void WebSocketConnection::send(WebSocketOpcode opcode, std::string payload) {
  {
    std::lock_guard lock(mutex_);
    commands_.push_back({opcode, std::move(payload)});
  }
  wake();
}

void WebSocketConnection::close(int code, std::string reason) {
  send(WebSocketOpcode::Close, encodeWebSocketClose(code, reason));
}

void WebSocketConnection::wake() const {
  const std::uint64_t one = 1;
  [[maybe_unused]] auto ignored = ::write(wakeFd_, &one, sizeof(one));
}

std::deque<WebSocketConnection::Command> WebSocketConnection::takeCommands() {
  std::lock_guard lock(mutex_);
  return std::exchange(commands_, {});
}

void WebSocketConnection::report(const std::function<void()> &callback) const {
  if (!stopping_) callback();
}

void WebSocketConnection::run() {
  pthread_setname_np(pthread_self(), "rn-websocket");
  try {
    const auto endpoint = parseWebSocketUrl(request_.url);
    Session session(endpoint.connectUrl, stopping_);
    const auto key = makeWebSocketKey();
    session.queueRaw(buildWebSocketHandshake(endpoint, key, request_.protocols, request_.headers));
    const auto protocol = handshake(session, key);
    if (stopping_) return;
    report([&] { listener_.onOpen(protocol); });
    exchange(session);
  } catch (const std::exception &error) {
    const std::string message = error.what();
    report([&] { listener_.onFailed(message); });
  }
}

std::string WebSocketConnection::handshake(Session &session, std::string_view key) {
  const auto deadline = Clock::now() + kHandshakeTimeout;
  while (!stopping_) {
    session.flush();
    const bool open = session.read();
    if (const auto end = session.inbox.find("\r\n\r\n"); end != std::string::npos) {
      auto protocol = checkWebSocketHandshake(std::string_view(session.inbox).substr(0, end), key, request_.protocols);
      session.inbox.erase(0, end + 4);
      return protocol;
    }
    if (!open) throw std::runtime_error("The server closed the connection during the handshake");
    if (session.inbox.size() > kMaxHandshakeBytes) throw std::runtime_error("The handshake response is too long");
    if (Clock::now() >= deadline) throw std::runtime_error("The server did not answer the handshake");
    session.wait(wakeFd_, deadline);
  }
  return {};
}

void WebSocketConnection::exchange(Session &session) {
  while (true) {
    for (auto &command : takeCommands()) {
      if (session.closeSent) continue;
      if (command.opcode == WebSocketOpcode::Close) {
        session.closeSent = decodeWebSocketClose(command.payload);
        session.closeDeadline = Clock::now() + kCloseTimeout;
      }
      session.queue(command.opcode, command.payload);
    }
    if (stopping_) {
      if (!session.closeSent) session.queue(WebSocketOpcode::Close, encodeWebSocketClose(kGoingAway, {}));
      session.flush();
      return;
    }
    session.flush();
    const bool open = session.read();
    while (auto frame = takeWebSocketFrame(session.inbox, kMaxMessageBytes)) {
      switch (frame->opcode) {
        case WebSocketOpcode::Ping:
          if (!session.closeSent) session.queue(WebSocketOpcode::Pong, frame->payload);
          break;
        case WebSocketOpcode::Pong:
          break;
        case WebSocketOpcode::Close: {
          auto [code, reason] = decodeWebSocketClose(frame->payload);
          if (!session.closeSent) {
            session.queue(WebSocketOpcode::Close, code == kNoStatus ? std::string() : encodeWebSocketClose(code, {}));
            session.drain(wakeFd_, Clock::now() + kCloseTimeout);
          }
          report([&] { listener_.onClosed(code, reason); });
          return;
        }
        case WebSocketOpcode::Text:
        case WebSocketOpcode::Binary:
          if (session.fragmentOpcode) throw std::runtime_error("The server started a message inside another");
          if (frame->final) {
            report([&] { listener_.onMessage(std::move(frame->payload), frame->opcode == WebSocketOpcode::Binary); });
          } else {
            session.fragmentOpcode = frame->opcode;
            session.fragments = std::move(frame->payload);
          }
          break;
        case WebSocketOpcode::Continuation:
          if (!session.fragmentOpcode) throw std::runtime_error("The server continued a message it never started");
          session.fragments += frame->payload;
          if (session.fragments.size() > kMaxMessageBytes)
            throw std::runtime_error("The server sent a message too long");
          if (frame->final) {
            const bool binary = *session.fragmentOpcode == WebSocketOpcode::Binary;
            session.fragmentOpcode.reset();
            report([&] { listener_.onMessage(std::exchange(session.fragments, {}), binary); });
          }
          break;
      }
    }
    if (!open) {
      if (!session.closeSent) throw std::runtime_error("The server closed the connection");
      report([&] { listener_.onClosed(session.closeSent->first, session.closeSent->second); });
      return;
    }
    if (session.closeSent && Clock::now() >= session.closeDeadline) {
      report([&] { listener_.onClosed(session.closeSent->first, session.closeSent->second); });
      return;
    }
    session.wait(wakeFd_, session.closeSent ? std::optional(session.closeDeadline) : std::nullopt);
  }
}

} // namespace rntizen
