#include "WebSocketConnection.h"

#include "NetworkError.h"
#include "WebSocketHandshake.h"
#include "WebSocketTransport.h"

#include <poll.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <random>

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
  if (!deadline.has_value()) return -1;
  const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(*deadline - Clock::now()).count();
  return static_cast<int>(std::max<std::int64_t>(left, 0));
}

} // namespace

struct WebSocketConnection::Incoming {
  std::string inbox;
  std::optional<WebSocketOpcode> fragmentOpcode;
  std::string fragments;
};

struct WebSocketConnection::Closing {
  int code;
  std::string reason;
  Clock::time_point deadline;
};

class WebSocketConnection::Session {
 public:
  Session(const std::string &url, const std::atomic<bool> &abort) : transport_(url, abort) {}

  void queueRaw(std::string bytes) {
    outbox_.push_back(std::move(bytes));
  }

  void queue(WebSocketOpcode opcode, std::string_view payload) {
    const auto bits = entropy_();
    const std::array mask{
        static_cast<std::uint8_t>(bits),
        static_cast<std::uint8_t>(bits >> 8),
        static_cast<std::uint8_t>(bits >> 16),
        static_cast<std::uint8_t>(bits >> 24),
    };
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

  bool read(std::string &inbox) {
    std::array<char, kReadChunk> chunk{};
    while (true) {
      const auto received = transport_.receive(chunk.data(), chunk.size());
      if (!received.has_value()) return true;
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

 private:
  WebSocketTransport transport_;
  std::deque<std::string> outbox_;
  std::size_t sentOfFront_{0};
  std::random_device entropy_;
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
    commands_.emplace_back(opcode, std::move(payload));
  }
  wake();
}

void WebSocketConnection::close(int code, std::string_view reason) {
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

void WebSocketConnection::reportMessage(std::string data, bool binary) const {
  if (!stopping_) listener_.onMessage(std::move(data), binary);
}

void WebSocketConnection::reportClosed(int code, std::string reason) const {
  if (!stopping_) listener_.onClosed(code, std::move(reason));
}

void WebSocketConnection::run() {
  pthread_setname_np(pthread_self(), "rn-websocket");
  try {
    const auto endpoint = parseWebSocketUrl(request_.url);
    const auto key = makeWebSocketKey();
    auto opening = buildWebSocketHandshake(endpoint, key, request_.protocols, request_.headers);
    Session session(endpoint.connectUrl, stopping_);
    session.queueRaw(std::move(opening));
    Incoming incoming;
    const auto protocol = handshake(session, incoming.inbox, key);
    if (stopping_) return;
    listener_.onOpen(protocol);
    exchange(session, incoming);
  } catch (const std::exception &error) {
    if (!stopping_) listener_.onFailed(error.what());
  }
}

std::string WebSocketConnection::handshake(Session &session, std::string &inbox, std::string_view key) const {
  const auto deadline = Clock::now() + kHandshakeTimeout;
  while (!stopping_) {
    session.flush();
    const bool open = session.read(inbox);
    if (const auto end = inbox.find("\r\n\r\n"); end != std::string::npos) {
      auto protocol = checkWebSocketHandshake(std::string_view(inbox).substr(0, end), key, request_.protocols);
      inbox.erase(0, end + 4);
      return protocol;
    }
    if (!open) throw NetworkError("The server closed the connection during the handshake");
    if (inbox.size() > kMaxHandshakeBytes) throw NetworkError("The handshake response is too long");
    if (Clock::now() >= deadline) throw NetworkError("The server did not answer the handshake");
    session.wait(wakeFd_, deadline);
  }
  return {};
}

void WebSocketConnection::exchange(Session &session, Incoming &incoming) {
  std::optional<Closing> closing;
  while (true) {
    queueCommands(session, closing);
    if (stopping_) {
      if (!closing.has_value()) session.queue(WebSocketOpcode::Close, encodeWebSocketClose(kGoingAway, {}));
      session.flush();
      return;
    }
    session.flush();
    const bool open = session.read(incoming.inbox);
    if (readFrames(session, incoming, closing)) return;
    if (!open && !closing.has_value()) throw NetworkError("The server closed the connection");
    if (!open || (closing.has_value() && Clock::now() >= closing->deadline)) {
      reportClosed(closing->code, closing->reason);
      return;
    }
    session.wait(wakeFd_, closing.has_value() ? std::optional(closing->deadline) : std::nullopt);
  }
}

void WebSocketConnection::queueCommands(Session &session, std::optional<Closing> &closing) {
  for (const auto &command : takeCommands()) {
    if (closing.has_value()) continue;
    if (command.opcode == WebSocketOpcode::Close) {
      auto [code, reason] = decodeWebSocketClose(command.payload);
      closing = Closing{.code = code, .reason = std::move(reason), .deadline = Clock::now() + kCloseTimeout};
    }
    session.queue(command.opcode, command.payload);
  }
}

bool WebSocketConnection::readFrames(Session &session, Incoming &incoming, const std::optional<Closing> &closing) const {
  using enum WebSocketOpcode;
  while (auto frame = takeWebSocketFrame(incoming.inbox, kMaxMessageBytes)) {
    switch (frame->opcode) {
      case Close:
        onServerClose(session, *frame, closing);
        return true;
      case Ping:
        if (!closing.has_value()) session.queue(Pong, frame->payload);
        break;
      case Pong:
        break;
      case Text:
      case Binary:
      case Continuation:
        onMessageFrame(incoming, *frame);
        break;
    }
  }
  return false;
}

void WebSocketConnection::onServerClose(
    Session &session,
    const WebSocketFrame &frame,
    const std::optional<Closing> &closing) const {
  auto [code, reason] = decodeWebSocketClose(frame.payload);
  if (!closing.has_value()) {
    session.queue(WebSocketOpcode::Close, code == kNoStatus ? std::string() : encodeWebSocketClose(code, {}));
    session.drain(wakeFd_, Clock::now() + kCloseTimeout);
  }
  reportClosed(code, std::move(reason));
}

void WebSocketConnection::onMessageFrame(Incoming &incoming, WebSocketFrame &frame) const {
  if (frame.opcode != WebSocketOpcode::Continuation) {
    if (incoming.fragmentOpcode.has_value()) throw NetworkError("The server started a message inside another");
    if (frame.isFinal) {
      reportMessage(std::move(frame.payload), frame.opcode == WebSocketOpcode::Binary);
      return;
    }
    incoming.fragmentOpcode = frame.opcode;
    incoming.fragments = std::move(frame.payload);
    return;
  }
  if (!incoming.fragmentOpcode.has_value()) throw NetworkError("The server continued a message it never started");
  incoming.fragments += frame.payload;
  if (incoming.fragments.size() > kMaxMessageBytes) throw NetworkError("The server sent a message too long");
  if (!frame.isFinal) return;
  const bool binary = *incoming.fragmentOpcode == WebSocketOpcode::Binary;
  incoming.fragmentOpcode.reset();
  reportMessage(std::exchange(incoming.fragments, {}), binary);
}

} // namespace rntizen
