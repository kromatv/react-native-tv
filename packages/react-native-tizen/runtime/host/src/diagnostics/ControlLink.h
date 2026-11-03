#pragma once

#include "ScreenCapture.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <variant>

namespace rntizen {

/// The connection `rn-tizen screenshot` and `rn-tizen perf` reach a development
/// build through: a second TCP connection to the log endpoint, beside the log
/// stream, held open, answering screenshot requests with the window's pixels
/// and taking perf requests. It reconnects every 500 ms, so a listener started
/// after the app finds it at once.
class ControlLink final {
 public:
  ControlLink(std::string host, std::uint16_t port);
  ~ControlLink();

  ControlLink(const ControlLink &) = delete;
  ControlLink &operator=(const ControlLink &) = delete;

  /// Whether the listener asked for perf samples: from its request until it
  /// turns them off or the link drops.
  bool perfRequested() const;

 private:
  void run();
  void serve(int socket);
  bool answer(int socket, const std::string &line);
  std::variant<ScreenCapture, std::string> capture();
  bool isStopping();

  const std::string host_;
  const std::uint16_t port_;
  std::mutex mutex_;
  std::condition_variable wake_;
  bool stopping_{false};
  std::atomic<bool> perfRequested_{false};
  int socket_{-1};
  std::thread thread_;
};

} // namespace rntizen
