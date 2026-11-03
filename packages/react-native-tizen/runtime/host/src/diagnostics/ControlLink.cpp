#include "ControlLink.h"

#include "../logging/Log.h"
#include "../logging/TcpConnection.h"
#include "../threading/MainThread.h"

#include <folly/dynamic.h>
#include <folly/json.h>

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <future>
#include <memory>
#include <optional>

namespace rntizen {

namespace {

constexpr auto kReconnectDelay = std::chrono::milliseconds(500);
constexpr auto kCaptureTimeout = std::chrono::seconds(5);
constexpr auto kStopPollInterval = std::chrono::milliseconds(250);
constexpr auto kConnectTimeout = std::chrono::seconds(2);
constexpr std::size_t kMaxRequestBytes = 4096;

bool sendLine(int socket, const folly::dynamic &message) {
  const auto line = folly::toJson(message) + "\n";
  return sendAll(socket, line.data(), line.size());
}

std::optional<std::int64_t> screenshotRequestId(const std::string &line) {
  try {
    const auto request = folly::parseJson(line);
    if (!request.isObject()) return std::nullopt;
    const auto *type = request.get_ptr("type");
    const auto *id = request.get_ptr("id");
    if (type == nullptr || !type->isString() || type->getString() != "screenshot") return std::nullopt;
    if (id == nullptr || !id->isInt() || id->getInt() < 0) return std::nullopt;
    return id->getInt();
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

std::optional<bool> perfRequest(const std::string &line) {
  try {
    const auto request = folly::parseJson(line);
    if (!request.isObject()) return std::nullopt;
    const auto *type = request.get_ptr("type");
    const auto *on = request.get_ptr("on");
    if (type == nullptr || !type->isString() || type->getString() != "perf") return std::nullopt;
    if (on == nullptr || !on->isBool()) return std::nullopt;
    return on->getBool();
  } catch (const std::exception &) {
    return std::nullopt;
  }
}

} // namespace

ControlLink::ControlLink(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port), thread_([this] { run(); }) {}

ControlLink::~ControlLink() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
    if (socket_ >= 0) shutdown(socket_, SHUT_RDWR);
  }
  wake_.notify_all();
  thread_.join();
}

bool ControlLink::perfRequested() const {
  return perfRequested_.load(std::memory_order_relaxed);
}

bool ControlLink::isStopping() {
  std::lock_guard lock(mutex_);
  return stopping_;
}

void ControlLink::run() {
  while (!isStopping()) {
    const int socket = connectTcp(host_, port_, kConnectTimeout, [this] { return isStopping(); });
    if (socket >= 0) {
      bool adopted = false;
      {
        std::lock_guard lock(mutex_);
        adopted = !stopping_;
        if (adopted) socket_ = socket;
      }
      if (adopted && sendLine(socket, folly::dynamic::object("type", "control")("version", 1))) serve(socket);
      perfRequested_.store(false, std::memory_order_relaxed);
      {
        std::lock_guard lock(mutex_);
        socket_ = -1;
      }
      close(socket);
    }
    std::unique_lock lock(mutex_);
    wake_.wait_for(lock, kReconnectDelay, [this] { return stopping_; });
  }
}

void ControlLink::serve(int socket) {
  std::string pending;
  char chunk[512];
  while (true) {
    const ssize_t received = recv(socket, chunk, sizeof chunk, 0);
    if (received < 0 && errno == EINTR) continue;
    if (received <= 0) return;
    pending.append(chunk, static_cast<std::size_t>(received));
    for (auto newline = pending.find('\n'); newline != std::string::npos; newline = pending.find('\n')) {
      const std::string line = pending.substr(0, newline);
      pending.erase(0, newline + 1);
      if (!answer(socket, line)) return;
    }
    if (pending.size() > kMaxRequestBytes) return;
  }
}

bool ControlLink::answer(int socket, const std::string &line) {
  if (const auto perf = perfRequest(line)) {
    perfRequested_.store(*perf, std::memory_order_relaxed);
    return true;
  }
  const auto id = screenshotRequestId(line);
  if (!id) return true;
  auto outcome = capture();
  if (const auto *failure = std::get_if<std::string>(&outcome)) {
    log(LogLevel::Warning, "Screenshot failed: " + *failure);
    return sendLine(socket, folly::dynamic::object("type", "screenshot-error")("id", *id)("message", *failure));
  }
  const auto &captured = std::get<ScreenCapture>(outcome);
  log(LogLevel::Debug,
      "Screenshot " + std::to_string(captured.width) + "x" + std::to_string(captured.height) + " sent");
  const folly::dynamic header = folly::dynamic::object("type", "screenshot")("id", *id)("width", captured.width)(
      "height", captured.height)("stride", captured.stride)("bytes", static_cast<std::int64_t>(captured.pixels.size()));
  return sendLine(socket, header) && sendAll(socket, captured.pixels.data(), captured.pixels.size());
}

std::variant<ScreenCapture, std::string> ControlLink::capture() {
  auto promise = std::make_shared<std::promise<std::optional<ScreenCapture>>>();
  auto future = promise->get_future();
  runOnMainThread([promise] {
    captureHostWindow([promise](std::optional<ScreenCapture> captured) { promise->set_value(std::move(captured)); });
  });
  const auto deadline = std::chrono::steady_clock::now() + kCaptureTimeout;
  while (future.wait_for(kStopPollInterval) != std::future_status::ready) {
    if (isStopping()) return std::string("the app is closing");
    if (std::chrono::steady_clock::now() >= deadline) {
      return std::string("the app drew no frame within 5 s, as when it is in the background");
    }
  }
  auto captured = future.get();
  if (!captured) return std::string("Evas gave no pixels for the window");
  return std::move(*captured);
}

} // namespace rntizen
