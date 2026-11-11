#include "SimulatorControl.h"

#include "../input/RemoteKeyMap.h"
#include "../logging/Log.h"
#include "../threading/MainThread.h"
#include "MainLoop.h"

#include <SDL3/SDL.h>
#include <fcntl.h>
#include <folly/json.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <string_view>
#include <utility>

namespace rnwebos {

namespace {

constexpr std::size_t kMaxLine = 4096;
constexpr auto kPressLength = std::chrono::milliseconds(60);

void sendKey(SDL_Scancode scancode, bool down) {
  SDL_Event event{};
  event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
  event.key.timestamp = SDL_GetTicksNS();
  event.key.scancode = scancode;
  event.key.key = SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, false);
  event.key.down = down;
  SDL_PushEvent(&event);
}

void pressKey(SDL_Scancode scancode, std::chrono::milliseconds length) {
  sendKey(scancode, true);
  MainLoop::get().addTimer(length, [scancode] { sendKey(scancode, false); });
}

std::pair<std::string_view, std::string_view> splitWord(std::string_view text) {
  const auto space = text.find(' ');
  if (space == std::string_view::npos) return {text, {}};
  return {text.substr(0, space), text.substr(space + 1)};
}

void runKeyCommand(std::string_view command, std::string_view arguments) {
  const auto [name, rest] = splitWord(arguments);
  const auto scancode = scancodeForKeyName(name);
  if (scancode == SDL_SCANCODE_UNKNOWN) {
    log(LogLevel::Warning, "The simulator has no remote key named " + std::string(name));
    return;
  }
  if (command == "key-down") return sendKey(scancode, true);
  if (command == "key-up") return sendKey(scancode, false);
  auto length = kPressLength;
  if (command == "key-hold") {
    int milliseconds = 0;
    std::from_chars(rest.data(), rest.data() + rest.size(), milliseconds);
    if (milliseconds > 0) length = std::chrono::milliseconds(milliseconds);
  }
  pressKey(scancode, length);
}

void runRelaunch(const SimulatorControl::Host &host, std::string_view json) {
  try {
    const auto parameters = json.empty() ? folly::dynamic(folly::dynamic::object()) : folly::parseJson(json);
    if (parameters.isObject()) return host.relaunch(parameters);
  } catch (const std::exception &) {
  }
  log(LogLevel::Warning, "The parameters of a relaunch are a JSON object, not: " + std::string(json));
}

void runCommand(const SimulatorControl::Host &host, std::string_view line) {
  const auto [command, arguments] = splitWord(line);
  if (command == "pause") return host.pause();
  if (command == "resume") return host.resume();
  if (command == "exit") return host.exit();
  if (command == "open-url") return host.openUrl(std::string(arguments));
  if (command == "relaunch") return runRelaunch(host, arguments);
  if (command.starts_with("key")) return runKeyCommand(command, arguments);
  log(LogLevel::Warning, "Unknown simulator command: " + std::string(line));
}

} // namespace

std::unique_ptr<SimulatorControl> SimulatorControl::open(Host host) {
  const char *path = std::getenv("RN_WEBOS_SIMULATOR_CONTROL");
  if (path == nullptr || *path == '\0') return nullptr;
  // Read and write, so the FIFO never reports end of file between two writers.
  const int fifo = ::open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (fifo < 0) {
    log(LogLevel::Warning, std::string("Cannot open the simulator control FIFO ") + path);
    return nullptr;
  }
  return std::unique_ptr<SimulatorControl>(
      new SimulatorControl(fifo, eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK), std::move(host)));
}

SimulatorControl::SimulatorControl(int fifo, int wake, Host host)
    : fifo_(fifo), wake_(wake), host_(std::make_shared<Host>(std::move(host))), thread_([this] { run(); }) {}

SimulatorControl::~SimulatorControl() {
  thread_.request_stop();
  const std::uint64_t one = 1;
  if (write(wake_, &one, sizeof one) < 0) log(LogLevel::Debug, "Cannot wake the simulator control thread");
  thread_.join();
  close(fifo_);
  close(wake_);
}

void SimulatorControl::run() {
  std::string pending;
  std::array<char, 1024> buffer{};
  const auto stop = thread_.get_stop_token();
  while (!stop.stop_requested()) {
    std::array<pollfd, 2> fds{{{.fd = fifo_, .events = POLLIN, .revents = 0}, {.fd = wake_, .events = POLLIN, .revents = 0}}};
    if (poll(fds.data(), fds.size(), -1) < 0) continue;
    if ((fds[0].revents & POLLIN) == 0) continue;
    const auto count = read(fifo_, buffer.data(), buffer.size());
    if (count <= 0) continue;
    pending.append(buffer.data(), static_cast<std::size_t>(count));
    for (auto end = pending.find('\n'); end != std::string::npos; end = pending.find('\n')) {
      auto line = pending.substr(0, end);
      pending.erase(0, end + 1);
      if (line.empty()) continue;
      runOnMainThread([host = host_, line = std::move(line)] { runCommand(*host, line); });
    }
    if (pending.size() > kMaxLine) pending.clear();
  }
}

} // namespace rnwebos
