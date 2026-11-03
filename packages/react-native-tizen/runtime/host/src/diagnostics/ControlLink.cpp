#include "ControlLink.h"

#include "../logging/Log.h"
#include "../logging/TcpConnection.h"
#include "../threading/MainThread.h"

#include <folly/dynamic.h>
#include <folly/json.h>

#include <sys/socket.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <stdexcept>
#include <chrono>
#include <format>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>

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

folly::dynamic parseRequest(const std::string &line) {
  try {
    return folly::parseJson(line);
  } catch (const folly::json::parse_error &) {
    return nullptr;
  } catch (const std::range_error &) {
    return nullptr;
  }
}

std::optional<std::int64_t> screenshotRequestId(const std::string &line) {
  const auto request = parseRequest(line);
  if (!request.isObject()) return std::nullopt;
  const auto *type = request.get_ptr("type");
  const auto *id = request.get_ptr("id");
  if (type == nullptr || !type->isString() || type->getString() != "screenshot") return std::nullopt;
  if (id == nullptr || !id->isInt() || id->getInt() < 0) return std::nullopt;
  return id->getInt();
}

std::optional<bool> perfRequest(const std::string &line) {
  const auto request = parseRequest(line);
  if (!request.isObject()) return std::nullopt;
  const auto *type = request.get_ptr("type");
  const auto *on = request.get_ptr("on");
  if (type == nullptr || !type->isString() || type->getString() != "perf") return std::nullopt;
  if (on == nullptr || !on->isBool()) return std::nullopt;
  return on->getBool();
}

std::optional<std::string> takeLine(std::string &pending) {
  const auto newline = pending.find('\n');
  if (newline == std::string::npos) return std::nullopt;
  auto line = pending.substr(0, newline);
  pending.erase(0, newline + 1);
  return line;
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
  return perfRequested_.load();
}

bool ControlLink::isStopping() {
  std::lock_guard lock(mutex_);
  return stopping_;
}

void ControlLink::run() {
  while (!isStopping()) {
    if (const int socket = connectTcp(host_, port_, kConnectTimeout, [this] { return isStopping(); }); socket >= 0) {
      if (adopt(socket) && sendLine(socket, folly::dynamic::object("type", "control")("version", 1))) serve(socket);
      perfRequested_.store(false);
      forget();
      close(socket);
    }
    waitToReconnect();
  }
}

bool ControlLink::adopt(int socket) {
  std::lock_guard lock(mutex_);
  if (stopping_) return false;
  socket_ = socket;
  return true;
}

void ControlLink::forget() {
  std::lock_guard lock(mutex_);
  socket_ = -1;
}

void ControlLink::waitToReconnect() {
  std::unique_lock lock(mutex_);
  wake_.wait_for(lock, kReconnectDelay, [this] { return stopping_; });
}

void ControlLink::serve(int socket) {
  std::string pending;
  std::array<char, 512> chunk{};
  while (true) {
    const ssize_t received = recv(socket, chunk.data(), chunk.size(), 0);
    if (received < 0 && errno == EINTR) continue;
    if (received <= 0) return;
    pending.append(chunk.data(), static_cast<std::size_t>(received));
    while (const auto line = takeLine(pending)) {
      if (!answer(socket, *line)) return;
    }
    if (pending.size() > kMaxRequestBytes) return;
  }
}

bool ControlLink::answer(int socket, const std::string &line) {
  if (const auto perf = perfRequest(line); perf.has_value()) {
    perfRequested_.store(*perf);
    return true;
  }
  const auto id = screenshotRequestId(line);
  if (!id.has_value()) return true;
  auto outcome = capture();
  if (const auto *failure = std::get_if<std::string>(&outcome)) {
    log(LogLevel::Warning, "Screenshot failed: " + *failure);
    return sendLine(socket, folly::dynamic::object("type", "screenshot-error")("id", *id)("message", *failure));
  }
  const auto &captured = std::get<ScreenCapture>(outcome);
  log(LogLevel::Debug, std::format("Screenshot {}x{} sent", captured.width, captured.height));
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
