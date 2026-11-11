#pragma once

#include "Log.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>

namespace rntizen {

/// Streams log lines over TCP to the machine that launched the app, one JSON
/// object per line (`{"level":"info","message":"..."}`), which `rn-tizen logs`
/// prints. Each connection opens with `{"type":"log","version":1,"app":<id>}`,
/// naming the package the lines come from. Writing never blocks: lines queue for a background thread. While
/// nothing listens, the sink retries every 500 ms and keeps the last 200 lines
/// at info or louder; the listener it then reaches first reads how many lines
/// it missed. A listener that went away is noticed before the next line is
/// written, so that line goes to the next listener.
class NetworkLogSink final : public LogSink {
 public:
  NetworkLogSink(std::string host, std::uint16_t port, const std::string &app);
  ~NetworkLogSink() override;

  NetworkLogSink(const NetworkLogSink &) = delete;
  NetworkLogSink &operator=(const NetworkLogSink &) = delete;

  void write(LogLevel level, std::string_view message) override;

 private:
  void run();
  bool ensureConnected(std::unique_lock<std::mutex> &lock);
  void adopt(int socket);
  void disconnect();
  void markUnreachable();
  bool isStopping();

  const std::string host_;
  const std::uint16_t port_;
  const std::string hello_;
  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::string> pending_;
  std::size_t dropped_{0};
  std::size_t sentOnSocket_{0};
  bool greeted_{false};
  bool unreachable_{false};
  bool stopping_{false};
  int socket_{-1};
  std::jthread thread_;
};

} // namespace rntizen
