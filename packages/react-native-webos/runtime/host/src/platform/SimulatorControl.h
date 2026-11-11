#pragma once

#include <folly/dynamic.h>

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace rnwebos {

/// What `rn-webos simulator` tells a running app, read from the FIFO named by
/// `RN_WEBOS_SIMULATOR_CONTROL`, one command per line: `pause`, `resume`,
/// `exit`, `open-url <url>`, `relaunch <json>` with the parameters of a launch
/// of the running app, and `key <name>`, `key-down <name>`,
/// `key-up <name>` or `key-hold <name> <milliseconds>` for the remote. A key
/// becomes the SDL key event the TV's remote would send, so it takes the same
/// path as on a TV. Simulator builds only.
class SimulatorControl final {
 public:
  struct Host {
    std::function<void()> pause;
    std::function<void()> resume;
    std::function<void()> exit;
    std::function<void(const std::string &url)> openUrl;
    std::function<void(const folly::dynamic &parameters)> relaunch;
  };

  /// Starts reading the FIFO; null when the environment names none or it cannot be opened.
  static std::unique_ptr<SimulatorControl> open(Host host);
  ~SimulatorControl();

  SimulatorControl(const SimulatorControl &) = delete;
  SimulatorControl &operator=(const SimulatorControl &) = delete;

 private:
  SimulatorControl(int fifo, int wake, Host host);
  void run();

  const int fifo_;
  const int wake_;
  const std::shared_ptr<Host> host_;
  std::jthread thread_;
};

} // namespace rnwebos
