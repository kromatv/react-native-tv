#include "NetworkLogSink.h"

#include "../diagnostics/CrashReporter.h"
#include "TcpConnection.h"

#include <folly/dynamic.h>
#include <folly/json.h>

#include <unistd.h>

#include <chrono>

namespace rntizen {

namespace {

constexpr std::size_t kMaxQueuedLines = 2000;
constexpr std::size_t kBacklogLines = 200;
constexpr auto kRetryDelay = std::chrono::milliseconds(500);
constexpr auto kConnectTimeout = std::chrono::seconds(2);

std::string jsonLine(LogLevel level, std::string_view message) {
  const folly::dynamic entry =
      folly::dynamic::object("level", std::string(logLevelName(level)))("message", std::string(message));
  return folly::toJson(entry) + "\n";
}

} // namespace

NetworkLogSink::NetworkLogSink(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port), thread_([this] { run(); }) {}

NetworkLogSink::~NetworkLogSink() {
  {
    std::lock_guard lock(mutex_);
    stopping_ = true;
  }
  wake_.notify_all();
  thread_.join();
  disconnect();
}

void NetworkLogSink::write(LogLevel level, std::string_view message) {
  auto line = jsonLine(level, message);
  std::lock_guard lock(mutex_);
  if (unreachable_ && level == LogLevel::Debug) {
    ++dropped_;
    return;
  }
  const auto limit = unreachable_ ? kBacklogLines : kMaxQueuedLines;
  while (pending_.size() >= limit) {
    pending_.pop_front();
    ++dropped_;
  }
  pending_.push_back(std::move(line));
  wake_.notify_one();
}

bool NetworkLogSink::isStopping() {
  std::lock_guard lock(mutex_);
  return stopping_;
}

void NetworkLogSink::adopt(int socket) {
  socket_ = socket;
  sentOnSocket_ = 0;
  unreachable_ = false;
  setCrashReportDescriptor(socket_);
  if (dropped_ == 0) return;
  pending_.push_front(jsonLine(
      LogLevel::Info, std::to_string(dropped_) + " earlier log lines were dropped while nothing listened"));
  dropped_ = 0;
}

void NetworkLogSink::disconnect() {
  if (socket_ < 0) return;
  setCrashReportDescriptor(-1);
  close(socket_);
  socket_ = -1;
}

void NetworkLogSink::markUnreachable() {
  unreachable_ = true;
  while (pending_.size() > kBacklogLines) {
    pending_.pop_front();
    ++dropped_;
  }
}

void NetworkLogSink::run() {
  std::unique_lock lock(mutex_);
  while (true) {
    wake_.wait(lock, [this] { return stopping_ || !pending_.empty(); });
    if (pending_.empty() || (stopping_ && socket_ < 0)) return;
    if (socket_ < 0) {
      lock.unlock();
      const int socket = connectTcp(host_, port_, kConnectTimeout, [this] { return isStopping(); });
      lock.lock();
      if (socket < 0) {
        markUnreachable();
        wake_.wait_for(lock, kRetryDelay, [this] { return stopping_; });
        continue;
      }
      adopt(socket);
    }
    std::string line = std::move(pending_.front());
    pending_.pop_front();
    const int socket = socket_;
    lock.unlock();
    const bool delivered = !peerHasClosed(socket) && sendAll(socket, line.data(), line.size());
    lock.lock();
    if (delivered) {
      ++sentOnSocket_;
      continue;
    }
    pending_.push_front(std::move(line));
    const bool refused = sentOnSocket_ == 0;
    disconnect();
    if (refused) wake_.wait_for(lock, kRetryDelay, [this] { return stopping_; });
  }
}

} // namespace rntizen
