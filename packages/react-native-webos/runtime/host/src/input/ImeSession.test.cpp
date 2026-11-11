#include "ImeSession.h"

#include "../platform/MainLoop.h"

#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <string>
#include <string_view>
#include <vector>

namespace rnwebos {

namespace {

class Field final : public ImeClient {
 public:
  void imeCommit(std::string_view text) override {
    calls.push_back("commit " + std::string(text));
  }
  void imePreedit(std::string_view text, int cursor) override {
    calls.push_back("preedit " + std::string(text) + " " + std::to_string(cursor));
  }
  void imePanelHidden() override {
    calls.emplace_back("hidden");
  }

  std::vector<std::string> calls;
};

SDL_Event committed(const char *text) {
  SDL_Event event{};
  event.type = SDL_EVENT_TEXT_INPUT;
  event.text.text = text;
  return event;
}

SDL_Event composing(const char *text, Sint32 start) {
  SDL_Event event{};
  event.type = SDL_EVENT_TEXT_EDITING;
  event.edit.text = text;
  event.edit.start = start;
  event.edit.length = -1;
  return event;
}

SDL_Event keyboard(SDL_EventType type) {
  SDL_Event event{};
  event.type = type;
  return event;
}

class Window {
 public:
  Window() {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_InitSubSystem(SDL_INIT_VIDEO));
    window_ = SDL_CreateWindow("ime", 64, 64, SDL_WINDOW_HIDDEN);
    REQUIRE(window_ != nullptr);
  }
  ~Window() {
    SDL_DestroyWindow(window_);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  }
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  SDL_Window *get() const {
    return window_;
  }

 private:
  SDL_Window *window_{nullptr};
};

} // namespace

TEST_CASE("ImeSession gives committed text to the field it edits, and to none before or after") {
  ImeSession session(nullptr);
  Field field;

  CHECK(session.handleEvent(committed("early")));
  session.begin(field, ImeOptions{});
  CHECK(session.handleEvent(committed("abc")));
  session.end();
  CHECK(session.handleEvent(committed("late")));

  CHECK(field.calls == std::vector<std::string>{"commit abc"});
}

TEST_CASE("ImeSession gives a composition to the field with its cursor, unset included") {
  ImeSession session(nullptr);
  Field field;
  session.begin(field, ImeOptions{});

  session.handleEvent(composing("ka", 1));
  session.handleEvent(composing("kan", -1));

  CHECK(field.calls == std::vector<std::string>{"preedit ka 1", "preedit kan -1"});
}

TEST_CASE("ImeSession turns a composition emptied right before its commit into the commit alone") {
  ImeSession session(nullptr);
  Field field;
  session.begin(field, ImeOptions{});
  session.handleEvent(composing("ka", 2));

  session.handleEvent(composing("", 0));
  session.handleEvent(committed("\xE3\x81\x8B"));
  MainLoop::get().iterate();

  CHECK(field.calls == std::vector<std::string>{"preedit ka 2", "commit \xE3\x81\x8B"});
}

TEST_CASE("ImeSession ends a composition emptied with no commit once the queued events are handled") {
  ImeSession session(nullptr);
  Field field;
  session.begin(field, ImeOptions{});
  session.handleEvent(composing("ka", 2));

  session.handleEvent(composing("", 0));
  CHECK(field.calls.size() == 1);
  MainLoop::get().iterate();

  CHECK(field.calls == std::vector<std::string>{"preedit ka 2", "preedit  0"});
}

TEST_CASE("ImeSession drops an emptied composition when the edit ends before it is handled") {
  ImeSession session(nullptr);
  Field field;
  session.begin(field, ImeOptions{});

  session.handleEvent(composing("", 0));
  session.end();
  MainLoop::get().iterate();

  CHECK(field.calls.empty());
}

TEST_CASE("ImeSession knows the on-screen keyboard from SDL's shown and hidden events") {
  ImeSession session(nullptr);
  Field field;
  CHECK_FALSE(session.isAvailable());
  session.begin(field, ImeOptions{});
  CHECK_FALSE(session.isPanelShown());

  session.handleEvent(keyboard(SDL_EVENT_SCREEN_KEYBOARD_SHOWN));
  CHECK(session.isPanelShown());
  CHECK(session.isAvailable());

  session.handleEvent(keyboard(SDL_EVENT_SCREEN_KEYBOARD_HIDDEN));
  session.handleEvent(keyboard(SDL_EVENT_SCREEN_KEYBOARD_HIDDEN));

  CHECK_FALSE(session.isPanelShown());
  CHECK(session.isActive());
  CHECK(field.calls == std::vector<std::string>{"hidden"});
}

TEST_CASE("ImeSession lowers the keyboard by stopping text input, and keeps the field for the next raise") {
  ImeSession session(nullptr);
  Field field;
  session.begin(field, ImeOptions{});
  session.handleEvent(keyboard(SDL_EVENT_SCREEN_KEYBOARD_SHOWN));

  session.hidePanel();
  CHECK_FALSE(session.isPanelShown());
  CHECK_FALSE(session.isActive());
  session.handleEvent(committed("lost"));
  session.showPanel();
  session.handleEvent(committed("kept"));

  CHECK(session.isActive());
  CHECK(field.calls == std::vector<std::string>{"hidden", "commit kept"});
}

TEST_CASE("ImeSession raises no keyboard for a field that ended its edit") {
  ImeSession session(nullptr);
  Field field;
  session.begin(field, ImeOptions{});
  session.end();

  session.showPanel();

  CHECK_FALSE(session.isActive());
}

TEST_CASE("ImeSession moves the edit to the field that begins next") {
  ImeSession session(nullptr);
  Field first;
  Field second;
  session.begin(first, ImeOptions{});

  session.begin(second, ImeOptions{});
  session.handleEvent(committed("x"));

  CHECK(first.calls.empty());
  CHECK(second.calls == std::vector<std::string>{"commit x"});
}

TEST_CASE("ImeSession takes no event that is not a text input event") {
  ImeSession session(nullptr);

  CHECK_FALSE(session.handleEvent(keyboard(SDL_EVENT_KEY_DOWN)));
}

TEST_CASE("ImeSession starts and stops SDL's text input on its window") {
  const Window window;
  Field field;
  {
    ImeSession session(window.get());

    session.begin(field, imeOptionsFor({.keyboardType = "email-address", .multiline = true}));
    CHECK(SDL_TextInputActive(window.get()));

    session.hidePanel();
    CHECK_FALSE(SDL_TextInputActive(window.get()));

    session.showPanel();
    CHECK(SDL_TextInputActive(window.get()));
    session.caretMoved({.x = 10, .y = 20, .width = 300, .height = 40}, 25);
    SDL_Rect area{};
    int cursor = 0;
    REQUIRE(SDL_GetTextInputArea(window.get(), &area, &cursor));
    CHECK(area.x == 10);
    CHECK(area.y == 20);
    CHECK(area.w == 300);
    CHECK(area.h == 40);
    CHECK(cursor == 15);
  }

  CHECK_FALSE(SDL_TextInputActive(window.get()));
}

} // namespace rnwebos
