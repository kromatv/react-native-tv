#pragma once

#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace rnwebos {

/// What `rn-webos simulator` tells a running app, read from the FIFO named by
/// `RN_WEBOS_SIMULATOR_CONTROL`, one command per line: `pause`, `resume`,
/// `exit`, `open-url <url>`, `key <name>`, `key-down <name>`, `key-up <name>`
/// or `key-hold <name> <milliseconds>` for the remote (the name after `Shift+`
/// for a keyboard's key with Shift down), and `pointer-move <x> <y>` (window
/// pixels), `pointer-down`, `pointer-up`, `pointer-wheel <notches>` and
/// `pointer-leave` for its pointer. Each becomes the SDL event the TV would
/// send, so it takes the same path as on a TV. The TV's on-screen keyboard,
/// which X11 has none of, is played by `text <UTF-8 text>` (the keyboard
/// commits that text), `compose <UTF-8 text>` (it shows that text as its
/// composition; no text ends the composition) and `keyboard shown` or
/// `keyboard hidden`, each as the SDL event the keyboard would cause.
/// Simulator builds only.
class SimulatorControl final {
 public:
  struct Host {
    std::function<void()> pause;
    std::function<void()> resume;
    std::function<void()> exit;
    std::function<void(const std::string &url)> openUrl;
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
