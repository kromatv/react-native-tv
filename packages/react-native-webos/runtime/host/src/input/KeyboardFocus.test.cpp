#include "KeyboardFocus.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

namespace rnwebos {

namespace {

constexpr PixelSize kScreen{.width = 100, .height = 100};
constexpr Uint16 kRawOk = 352;
constexpr Uint16 kRawInfo = 358;

SDL_KeyboardEvent keyEvent(SDL_Scancode scancode, Uint16 raw = 0) {
  SDL_KeyboardEvent key{};
  key.scancode = scancode;
  key.raw = raw;
  return key;
}

SDL_Event textEvent(const char *text) {
  SDL_Event event{};
  event.type = SDL_EVENT_TEXT_INPUT;
  event.text.text = text;
  return event;
}

std::string nameOf(const SDL_KeyboardEvent &key) {
  return key.scancode != SDL_SCANCODE_UNKNOWN ? std::to_string(key.scancode) : "raw " + std::to_string(key.raw);
}

class Field final : public KeyboardClient {
 public:
  bool keyDown(const SDL_KeyboardEvent &key, bool repeat) override {
    keys.push_back((repeat ? "repeat " : "down ") + nameOf(key));
    return takesKeys;
  }
  void keyUp(const SDL_KeyboardEvent &key) override {
    keys.push_back("up " + nameOf(key));
  }
  void keyboardLost() override {
    ++lost;
  }
  void imeCommit(std::string_view text) override {
    committed += text;
  }
  void imePreedit(std::string_view, int) override {}
  void imePanelHidden() override {}

  bool takesKeys{true};
  std::vector<std::string> keys;
  std::string committed;
  int lost{0};
};

const std::string kLeft = std::to_string(SDL_SCANCODE_LEFT);
const std::string kRight = std::to_string(SDL_SCANCODE_RIGHT);

} // namespace

TEST_CASE("KeyboardFocus::of finds a scene's keyboard focus until it is destroyed") {
  Scene scene(kScreen);
  Scene other(kScreen);

  {
    KeyboardFocus focus(scene);

    CHECK(KeyboardFocus::of(scene) == &focus);
    CHECK(KeyboardFocus::of(other) == nullptr);
  }

  CHECK(KeyboardFocus::of(scene) == nullptr);
}

TEST_CASE("KeyboardFocus leaves every key to the remote while no field has the keyboard") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  const auto ok = keyEvent(SDL_SCANCODE_RETURN);

  CHECK_FALSE(focus.handleKey(ok, true));
  CHECK_FALSE(focus.handleKey(ok, false));
}

TEST_CASE("KeyboardFocus gives the focused field its keys, repeats and releases included, until it blurs") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  const auto left = keyEvent(SDL_SCANCODE_LEFT);

  focus.focus(field, ImeOptions{});
  CHECK(focus.handleKey(left, true));
  CHECK(focus.handleKey(left, true));
  CHECK(focus.handleKey(left, false));
  focus.blur(field);

  CHECK(field.keys == std::vector<std::string>{"down " + kLeft, "repeat " + kLeft, "up " + kLeft});
  CHECK_FALSE(focus.hasFocus(field));
  CHECK_FALSE(focus.handleKey(left, true));
}

TEST_CASE("KeyboardFocus passes on a key the field leaves, and tells a field it lost the keyboard") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field first;
  Field second;
  first.takesKeys = false;

  focus.focus(first, ImeOptions{});
  CHECK_FALSE(focus.handleKey(keyEvent(SDL_SCANCODE_DOWN), true));
  focus.focus(second, ImeOptions{});

  CHECK(first.lost == 1);
  CHECK(focus.hasFocus(second));
}

TEST_CASE("KeyboardFocus keeps the keyboard with a field focused twice, and a field without it cannot blur it") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  Field other;

  focus.focus(field, ImeOptions{});
  focus.focus(field, ImeOptions{});
  focus.blur(other);

  CHECK(focus.hasFocus(field));
  CHECK(field.lost == 0);
}

TEST_CASE("KeyboardFocus keeps a key the field took from the remote after the field blurs, release included") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  const auto right = keyEvent(SDL_SCANCODE_RIGHT);
  focus.focus(field, ImeOptions{});
  REQUIRE(focus.handleKey(right, true));

  focus.blur(field);

  CHECK(focus.handleKey(right, true));
  CHECK(focus.handleKey(right, false));
  CHECK(field.keys == std::vector<std::string>{"down " + kRight});
  CHECK_FALSE(focus.handleKey(right, false));
}

TEST_CASE("KeyboardFocus keeps a repeat the field refuses from the remote when the field took its key") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  const auto left = keyEvent(SDL_SCANCODE_LEFT);
  focus.focus(field, ImeOptions{});
  REQUIRE(focus.handleKey(left, true));
  field.takesKeys = false;

  CHECK(focus.handleKey(left, true));
  CHECK(focus.handleKey(left, false));

  CHECK(field.keys == std::vector<std::string>{"down " + kLeft, "repeat " + kLeft, "up " + kLeft});
}

TEST_CASE("KeyboardFocus tells two keys SDL has no scancode for apart by their Linux key codes") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  focus.focus(field, ImeOptions{});
  REQUIRE(focus.handleKey(keyEvent(SDL_SCANCODE_UNKNOWN, kRawOk), true));

  CHECK(focus.handleKey(keyEvent(SDL_SCANCODE_UNKNOWN, kRawInfo), true));
  CHECK(focus.handleKey(keyEvent(SDL_SCANCODE_UNKNOWN, kRawOk), false));

  CHECK(field.keys == std::vector<std::string>{"down raw 352", "down raw 358", "up raw 352"});
}

TEST_CASE("KeyboardFocus forgets the keys a field holds when told their releases may never come") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  const auto left = keyEvent(SDL_SCANCODE_LEFT);
  focus.focus(field, ImeOptions{});
  REQUIRE(focus.handleKey(left, true));

  focus.releaseKeys();

  CHECK_FALSE(focus.handleKey(left, false));
  CHECK(focus.handleKey(left, true));
  CHECK(field.keys == std::vector<std::string>{"down " + kLeft, "down " + kLeft});
}

TEST_CASE("KeyboardFocus gives SDL's text to the field that has the keyboard, and to none once it blurred") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  Field field;
  focus.focus(field, ImeOptions{});

  CHECK(focus.handleTextEvent(textEvent("abc")));
  focus.blur(field);
  CHECK(focus.handleTextEvent(textEvent("def")));

  CHECK(field.committed == "abc");
}

TEST_CASE("KeyboardFocus takes no event that is not a text input event") {
  Scene scene(kScreen);
  KeyboardFocus focus(scene);
  SDL_Event event{};
  event.type = SDL_EVENT_KEY_DOWN;

  CHECK_FALSE(focus.handleTextEvent(event));
}

} // namespace rnwebos
