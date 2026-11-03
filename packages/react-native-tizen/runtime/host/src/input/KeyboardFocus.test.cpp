#include "KeyboardFocus.h"

#include <doctest/doctest.h>

#include <Ecore_Evas.h>

#include <bit>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rntizen {

namespace {

Evas *fakeCanvas(std::uintptr_t address) {
  return std::bit_cast<Evas *>(address);
}

Ecore_Event_Key keyEvent(const char *name, unsigned int keycode) {
  Ecore_Event_Key key{};
  key.keyname = name;
  key.key = name;
  key.keycode = keycode;
  return key;
}

class Field final : public KeyboardClient {
 public:
  bool keyDown(const Ecore_Event_Key &key, bool repeat) override {
    keys.push_back(std::string(repeat ? "repeat " : "down ") + key.key);
    return takesKeys;
  }
  void keyUp(const Ecore_Event_Key &key) override {
    keys.push_back(std::string("up ") + key.key);
  }
  void keyboardLost() override {
    ++lost;
  }
  void imeCommit(std::string_view) override {}
  void imePreedit(std::string_view, int) override {}
  void imeDeleteSurrounding(int, int) override {}
  std::string imeSurrounding(int &cursor) const override {
    cursor = 0;
    return {};
  }
  void imePanelHidden() override {}

  bool takesKeys{true};
  std::vector<std::string> keys;
  int lost{0};
};

class Canvas {
 public:
  Canvas() {
    static const bool initialized = ecore_evas_init() > 0;
    REQUIRE(initialized);
    canvas_ = ecore_evas_buffer_new(100, 100);
  }
  ~Canvas() {
    ecore_evas_free(canvas_);
  }
  Canvas(const Canvas &) = delete;
  Canvas &operator=(const Canvas &) = delete;

  Evas *evas() const {
    return ecore_evas_get(canvas_);
  }

 private:
  Ecore_Evas *canvas_;
};

} // namespace

TEST_CASE("KeyboardFocus::of finds a canvas's keyboard focus until it is destroyed") {
  auto *canvas = fakeCanvas(0x1000);
  auto *other = fakeCanvas(0x2000);

  {
    KeyboardFocus focus(canvas);

    CHECK(KeyboardFocus::of(canvas) == &focus);
    CHECK(KeyboardFocus::of(other) == nullptr);
  }

  CHECK(KeyboardFocus::of(canvas) == nullptr);
}

TEST_CASE("KeyboardFocus leaves every key to the remote while no field has the keyboard") {
  KeyboardFocus focus(fakeCanvas(0x3000));
  Ecore_Event_Key key{};
  key.keyname = "Return";
  key.key = "Return";
  key.keycode = 36;

  CHECK_FALSE(focus.handleKey(key, true));
  CHECK_FALSE(focus.handleKey(key, false));
}

TEST_CASE("KeyboardFocus gives the focused field its keys, repeats and releases included, until it blurs") {
  const Canvas canvas;
  KeyboardFocus focus(canvas.evas());
  Field field;
  const auto left = keyEvent("Left", 113);

  focus.focus(field, ImeOptions{});
  CHECK(focus.handleKey(left, true));
  CHECK(focus.handleKey(left, true));
  CHECK(focus.handleKey(left, false));
  focus.blur(field);

  CHECK(field.keys == std::vector<std::string>{"down Left", "repeat Left", "up Left"});
  CHECK_FALSE(focus.hasFocus(field));
  CHECK_FALSE(focus.handleKey(left, true));
}

TEST_CASE("KeyboardFocus passes on a key the field leaves, and tells a field it lost the keyboard") {
  const Canvas canvas;
  KeyboardFocus focus(canvas.evas());
  Field first;
  Field second;
  first.takesKeys = false;

  focus.focus(first, ImeOptions{});
  CHECK_FALSE(focus.handleKey(keyEvent("Down", 116), true));
  focus.focus(second, ImeOptions{});

  CHECK(first.lost == 1);
  CHECK(focus.hasFocus(second));
}

} // namespace rntizen
