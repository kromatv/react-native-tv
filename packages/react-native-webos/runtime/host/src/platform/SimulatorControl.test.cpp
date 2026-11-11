#include "SimulatorControl.h"

#include "../threading/MainLoop.test.h"

#include <SDL3/SDL.h>
#include <doctest/doctest.h>
#include <fcntl.h>
#include <folly/json.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rnwebos {

namespace {

namespace fs = std::filesystem;
using std::chrono::milliseconds;

constexpr const char *kEnvironment = "RN_WEBOS_SIMULATOR_CONTROL";

struct TextEvent {
  Uint32 type;
  std::string text;

  bool operator==(const TextEvent &) const = default;
};

/// A control FIFO the environment names, with what the app would see of it:
/// the SDL events its loop hands on and the calls its host gets.
struct ControlChannel {
  const testing::MainLoopScope scope;
  const fs::path directory{fs::temp_directory_path() / ("rn-webos-control-test-" + std::to_string(getpid()))};
  const fs::path fifo{directory / "control"};
  std::vector<SDL_Event> events;
  std::vector<TextEvent> texts;
  std::vector<std::string> calls;
  std::atomic<bool> exited{false};
  std::unique_ptr<SimulatorControl> control;

  ControlChannel() {
    fs::create_directories(directory);
    REQUIRE(mkfifo(fifo.c_str(), 0600) == 0);
    setenv(kEnvironment, fifo.c_str(), 1);
    MainLoop::get().setEventHandler([this](const SDL_Event &event) { record(event); });
    control = SimulatorControl::open({
        .pause = [this] { calls.emplace_back("pause"); },
        .resume = [this] { calls.emplace_back("resume"); },
        .exit = [this] { exited = true; },
        .openUrl = [this](const std::string &url) { calls.push_back("open-url " + url); },
        .relaunch = [this](const folly::dynamic &parameters) { calls.push_back("relaunch " + folly::toJson(parameters)); },
    });
    REQUIRE(control != nullptr);
  }

  ~ControlChannel() {
    control.reset();
    MainLoop::get().setEventHandler(nullptr);
    unsetenv(kEnvironment);
    fs::remove_all(directory);
  }

  ControlChannel(const ControlChannel &) = delete;
  ControlChannel &operator=(const ControlChannel &) = delete;

  void record(const SDL_Event &event) {
    if (event.type == SDL_EVENT_TEXT_INPUT) {
      texts.push_back({event.type, event.text.text});
    } else if (event.type == SDL_EVENT_TEXT_EDITING) {
      texts.push_back({event.type, event.edit.text});
    } else {
      events.push_back(event);
    }
  }

  void write(std::string_view bytes) const {
    const int writer = open(fifo.c_str(), O_WRONLY | O_CLOEXEC);
    REQUIRE(writer >= 0);
    REQUIRE(::write(writer, bytes.data(), bytes.size()) == static_cast<ssize_t>(bytes.size()));
    close(writer);
  }

  /// Sends `lines`, then `exit`, and runs the loop until the host got that
  /// exit and for `settle` more, which lets the events of the last lines out.
  void send(std::string_view lines, milliseconds settle = milliseconds(20)) {
    exited = false;
    write(std::string(lines) + "exit\n");
    REQUIRE(scope.runUntil(exited));
    scope.runFor(settle);
  }
};

} // namespace

TEST_CASE("SimulatorControl opens only the FIFO the environment names") {
  unsetenv(kEnvironment);
  CHECK(SimulatorControl::open({}) == nullptr);

  setenv(kEnvironment, "", 1);
  CHECK(SimulatorControl::open({}) == nullptr);

  setenv(kEnvironment, "/nonexistent/rn-webos-control", 1);
  CHECK(SimulatorControl::open({}) == nullptr);
  unsetenv(kEnvironment);
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl runs pause, resume and open-url on the host, in the order written") {
  send("pause\nresume\nopen-url example://item/42\n");

  CHECK(calls == std::vector<std::string>{"pause", "resume", "open-url example://item/42"});
  CHECK(events.empty());
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl relaunches with a JSON object, an empty one without parameters, and with nothing else") {
  send("relaunch {\"contentTarget\":\"example://item/42\"}\nrelaunch\nrelaunch [1]\nrelaunch {oops\n");

  CHECK(calls == std::vector<std::string>{R"(relaunch {"contentTarget":"example://item/42"})", "relaunch {}"});
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl runs a line once it ends, skips empty ones and ignores an unknown command") {
  write("pau");
  scope.runFor(milliseconds(20));
  CHECK(calls.empty());

  send("se\n\nwarp 9\nresume\n");

  CHECK(calls == std::vector<std::string>{"pause", "resume"});
  CHECK(events.empty());
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl survives a line longer than it keeps") {
  write(std::string(6000, 'x'));
  scope.runFor(milliseconds(20));

  send("\npause\n");

  CHECK(calls == std::vector<std::string>{"pause"});
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl turns key-down and key-up into the remote's key events") {
  send("key-down Enter\nkey-up Enter\nkey-down Shift+Tab\nkey-down Volume\n");

  REQUIRE(events.size() == 3);
  CHECK(events[0].type == SDL_EVENT_KEY_DOWN);
  CHECK(events[0].key.scancode == SDL_SCANCODE_RETURN);
  CHECK(events[0].key.down);
  CHECK(events[0].key.mod == SDL_KMOD_NONE);
  CHECK(events[1].type == SDL_EVENT_KEY_UP);
  CHECK(events[1].key.scancode == SDL_SCANCODE_RETURN);
  CHECK_FALSE(events[1].key.down);
  CHECK(events[2].key.scancode == SDL_SCANCODE_TAB);
  CHECK(events[2].key.mod == SDL_KMOD_LSHIFT);
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl presses a key and releases it by itself, after the time key-hold names") {
  send("key Red\n", milliseconds(120));

  REQUIRE(events.size() == 2);
  CHECK(events[0].type == SDL_EVENT_KEY_DOWN);
  CHECK(events[0].key.scancode == SDL_SCANCODE_WEBOS_RED);
  CHECK(events[1].type == SDL_EVENT_KEY_UP);
  CHECK(events[1].key.scancode == SDL_SCANCODE_WEBOS_RED);

  events.clear();
  send("key-hold Enter 200\n", milliseconds(60));
  REQUIRE(events.size() == 1);
  CHECK(events[0].type == SDL_EVENT_KEY_DOWN);

  scope.runFor(milliseconds(250));
  REQUIRE(events.size() == 2);
  CHECK(events[1].type == SDL_EVENT_KEY_UP);
  CHECK(events[1].key.scancode == SDL_SCANCODE_RETURN);
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl types as the on-screen keyboard does: a composition, then the committed text") {
  send("compose kro\ntext kroma\ntext tv\ncompose\n");

  CHECK(texts == std::vector<TextEvent>{
                     {SDL_EVENT_TEXT_EDITING, "kro"},
                     {SDL_EVENT_TEXT_EDITING, ""},
                     {SDL_EVENT_TEXT_INPUT, "kroma"},
                     {SDL_EVENT_TEXT_INPUT, "tv"},
                     {SDL_EVENT_TEXT_EDITING, ""},
                 });
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl shows and hides the on-screen keyboard, and nothing for another state") {
  send("keyboard shown\nkeyboard hidden\nkeyboard sideways\n");

  REQUIRE(events.size() == 2);
  CHECK(events[0].type == SDL_EVENT_SCREEN_KEYBOARD_SHOWN);
  CHECK(events[1].type == SDL_EVENT_SCREEN_KEYBOARD_HIDDEN);
}

TEST_CASE_FIXTURE(ControlChannel, "SimulatorControl moves the pointer, and presses, releases and scrolls where it stands") {
  send("pointer-move 400 300.5\npointer-down\npointer-up\npointer-wheel -3\npointer-leave\npointer-spin\n");

  REQUIRE(events.size() == 5);
  CHECK(events[0].type == SDL_EVENT_MOUSE_MOTION);
  CHECK(events[0].motion.x == 400.0F);
  CHECK(events[0].motion.y == 300.5F);
  CHECK(events[1].type == SDL_EVENT_MOUSE_BUTTON_DOWN);
  CHECK(events[1].button.button == SDL_BUTTON_LEFT);
  CHECK(events[1].button.down);
  CHECK(events[1].button.x == 400.0F);
  CHECK(events[1].button.y == 300.5F);
  CHECK(events[2].type == SDL_EVENT_MOUSE_BUTTON_UP);
  CHECK_FALSE(events[2].button.down);
  CHECK(events[3].type == SDL_EVENT_MOUSE_WHEEL);
  CHECK(events[3].wheel.y == -3.0F);
  CHECK(events[3].wheel.mouse_x == 400.0F);
  CHECK(events[3].wheel.mouse_y == 300.5F);
  CHECK(events[4].type == SDL_EVENT_WINDOW_MOUSE_LEAVE);
}

} // namespace rnwebos
