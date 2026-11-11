#include "TextInputView.h"

#include "TextInputSurface.test.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace rnwebos {

namespace {

using testing::TextInputSurface;
using Events = std::vector<std::string>;

constexpr SDL_Keymod kShift = SDL_KMOD_LSHIFT;

bool has(const Events &events, const std::string &event) {
  return std::ranges::find(events, event) != events.end();
}

/// A surface whose field has the keyboard, with the events of getting there dropped.
class FocusedField {
 public:
  explicit FocusedField(const std::string &props = "{}") : surface(props) {
    surface.command("focus");
    surface.takeEvents();
  }

  TextInputSurface surface;
};

} // namespace

TEST_CASE("a field takes the keyboard on the focus command: focus, and blur then endEditing when it gives it back") {
  TextInputSurface surface("{text: 'abc'}");
  surface.takeEvents();

  surface.command("focus");
  CHECK(surface.keyboard().hasFocus(surface.field()));
  CHECK(surface.takeEvents() == Events{"topFocus {target:2}"});

  surface.command("blur");
  CHECK_FALSE(surface.keyboard().hasFocus(surface.field()));
  CHECK(surface.takeEvents() == Events{"topBlur {target:2}", "topEndEditing {target:2,text:\"abc\"}"});
}

TEST_CASE("autoFocus takes the keyboard when the field mounts, and a field that is not editable never does") {
  TextInputSurface autoFocused("{autoFocus: true}");
  TextInputSurface readOnly("{editable: false}");
  readOnly.takeEvents();

  readOnly.command("focus");

  CHECK(has(autoFocused.takeEvents(), "topFocus {target:2}"));
  CHECK_FALSE(readOnly.keyboard().hasFocus(readOnly.field()));
  CHECK(readOnly.takeEvents().empty());
}

TEST_CASE("typed text sends keyPress, change with event counts two apart, then selectionChange") {
  FocusedField focused;
  auto &surface = focused.surface;

  surface.commit("a");
  CHECK(
      surface.takeEvents() ==
      Events{
          "topKeyPress {key:\"a\",target:2}",
          "topChange {eventCount:2,selection:{end:1,start:1},target:2,text:\"a\"}",
          "topSelectionChange {selection:{end:1,start:1},target:2}",
      });

  surface.commit("b");
  CHECK(has(surface.takeEvents(), "topChange {eventCount:4,selection:{end:2,start:2},target:2,text:\"ab\"}"));
}

TEST_CASE("text longer than two UTF-16 units sends no keyPress, as a paste on Android") {
  FocusedField focused;

  focused.surface.commit("abc");

  const auto events = focused.surface.takeEvents();
  CHECK(has(events, "topChange {eventCount:2,selection:{end:3,start:3},target:2,text:\"abc\"}"));
  CHECK(events.front().starts_with("topChange"));
}

TEST_CASE("a text JavaScript sets replaces the field's without a change event and keeps the caret's place") {
  FocusedField focused("{text: 'abc', mostRecentEventCount: 0}");
  auto &surface = focused.surface;
  surface.press(SDL_SCANCODE_LEFT);
  surface.commit("x");
  REQUIRE(has(surface.takeEvents(), "topChange {eventCount:2,selection:{end:3,start:3},target:2,text:\"abxc\"}"));

  surface.setProps("{text: 'ABXC', mostRecentEventCount: 2}");
  CHECK(surface.takeEvents().empty());

  surface.commit("d");
  CHECK(has(surface.takeEvents(), "topChange {eventCount:4,selection:{end:4,start:4},target:2,text:\"ABXdC\"}"));
}

TEST_CASE("a text JavaScript sets with an event count older than the field's is ignored") {
  FocusedField focused;
  auto &surface = focused.surface;
  surface.commit("a");
  surface.commit("b");
  surface.takeEvents();

  surface.setProps("{text: 'A', mostRecentEventCount: 2}");
  surface.commit("c");

  CHECK(has(surface.takeEvents(), "topChange {eventCount:6,selection:{end:3,start:3},target:2,text:\"abc\"}"));
}

TEST_CASE("setTextAndSelection sets the text and the selection, unless its event count is older than the field's") {
  FocusedField focused;
  auto &surface = focused.surface;
  surface.commit("a");
  surface.takeEvents();

  surface.command("setTextAndSelection", "[0, 'stale', 0, 0]");
  CHECK(surface.takeEvents().empty());

  surface.command("setTextAndSelection", "[2, 'hello', 1, 3]");
  CHECK(
      surface.takeEvents() ==
      Events{
          "topSelectionChange {selection:{end:5,start:5},target:2}",
          "topSelectionChange {selection:{end:3,start:1},target:2}",
      });

  surface.commit("EY");
  CHECK(has(surface.takeEvents(), "topChange {eventCount:4,selection:{end:3,start:3},target:2,text:\"hEYlo\"}"));
}

TEST_CASE("setTextAndSelection with no text moves the selection only, and an unset end is the start") {
  FocusedField focused("{text: 'hello'}");
  auto &surface = focused.surface;

  surface.command("setTextAndSelection", "[0, null, 2, -1]");

  CHECK(surface.takeEvents() == Events{"topSelectionChange {selection:{end:2,start:2},target:2}"});
}

TEST_CASE("OK submits a single-line field and gives the keyboard back: submitEditing, blur, endEditing") {
  FocusedField focused("{text: 'query'}");
  auto &surface = focused.surface;

  CHECK(surface.press(SDL_SCANCODE_RETURN));

  CHECK(
      surface.takeEvents() ==
      Events{
          "topSubmitEditing {action:\"submit\",target:2,text:\"query\"}",
          "topBlur {target:2}",
          "topEndEditing {target:2,text:\"query\"}",
      });
}

TEST_CASE("a field that submits without blurring keeps the keyboard, and returnKeyType next names its action") {
  FocusedField focused("{text: 'a', submitBehavior: 'submit', returnKeyType: 'next'}");
  auto &surface = focused.surface;

  surface.press(SDL_SCANCODE_RETURN);

  CHECK(surface.takeEvents() == Events{"topSubmitEditing {action:\"next\",target:2,text:\"a\"}"});
  CHECK(surface.keyboard().hasFocus(surface.field()));
}

TEST_CASE("OK types a newline in a multiline field, which grows its content, and submits nothing") {
  FocusedField focused("{text: 'a', multiline: true}");
  auto &surface = focused.surface;

  surface.press(SDL_SCANCODE_RETURN);

  const auto events = surface.takeEvents();
  REQUIRE(events.size() == 4);
  CHECK(events[0] == "topKeyPress {key:\"Enter\",target:2}");
  CHECK(events[1] == "topChange {eventCount:2,selection:{end:2,start:2},target:2,text:\"a\\n\"}");
  CHECK(events[2] == "topSelectionChange {selection:{end:2,start:2},target:2}");
  CHECK(events[3].starts_with("topContentSizeChange {contentSize:{height:"));
  CHECK(surface.keyboard().hasFocus(surface.field()));
}

TEST_CASE("Backspace and Delete delete around the caret, and Backspace reports its key even with nothing to delete") {
  FocusedField focused("{text: 'abc'}");
  auto &surface = focused.surface;
  surface.press(SDL_SCANCODE_HOME);
  surface.takeEvents();

  surface.press(SDL_SCANCODE_BACKSPACE);
  CHECK(surface.takeEvents() == Events{"topKeyPress {key:\"Backspace\",target:2}"});

  surface.press(SDL_SCANCODE_DELETE);
  CHECK(surface.takeEvents() == Events{"topChange {eventCount:2,selection:{end:0,start:0},target:2,text:\"bc\"}"});

  surface.press(SDL_SCANCODE_END);
  surface.press(SDL_SCANCODE_BACKSPACE);
  CHECK(has(surface.takeEvents(), "topChange {eventCount:4,selection:{end:1,start:1},target:2,text:\"b\"}"));
}

TEST_CASE("the keyboard's Clear all empties the field") {
  FocusedField focused("{text: 'abc'}");

  focused.surface.press(SDL_SCANCODE_CLEAR);

  CHECK(has(focused.surface.takeEvents(), "topChange {eventCount:2,selection:{end:0,start:0},target:2,text:\"\"}"));
}

TEST_CASE("Shift and an arrow extend the selection, and typed text replaces it") {
  FocusedField focused("{text: 'hello'}");
  auto &surface = focused.surface;

  surface.press(SDL_SCANCODE_LEFT, kShift);
  surface.press(SDL_SCANCODE_LEFT, kShift);
  CHECK(
      surface.takeEvents() ==
      Events{
          "topSelectionChange {selection:{end:5,start:4},target:2}",
          "topSelectionChange {selection:{end:5,start:3},target:2}",
      });

  surface.commit("p");
  CHECK(has(surface.takeEvents(), "topChange {eventCount:2,selection:{end:4,start:4},target:2,text:\"help\"}"));
}

TEST_CASE("an arrow past the end of the text gives the key back to the remote and the keyboard with it") {
  FocusedField focused("{text: 'ab'}");
  auto &surface = focused.surface;

  CHECK_FALSE(surface.press(SDL_SCANCODE_RIGHT));

  CHECK(surface.takeEvents() == Events{"topBlur {target:2}", "topEndEditing {target:2,text:\"ab\"}"});
}

TEST_CASE("Up and Down leave a single-line field, as focus leaves an EditText on Android TV") {
  FocusedField focused("{text: 'ab'}");

  CHECK_FALSE(focused.surface.press(SDL_SCANCODE_DOWN));

  CHECK_FALSE(focused.surface.keyboard().hasFocus(focused.surface.field()));
}

TEST_CASE("maxLength keeps what fits and still reports the key of what does not") {
  FocusedField focused("{maxLength: 2}");
  auto &surface = focused.surface;
  surface.commit("a");
  surface.commit("b");
  surface.takeEvents();

  surface.commit("c");

  CHECK(surface.takeEvents() == Events{"topKeyPress {key:\"c\",target:2}"});
}

TEST_CASE("a composition changes the text at each step, and its commit is one change") {
  FocusedField focused;
  auto &surface = focused.surface;

  surface.compose("k");
  surface.compose("ka");
  CHECK(has(surface.takeEvents(), "topChange {eventCount:4,selection:{end:2,start:2},target:2,text:\"ka\"}"));

  surface.compose("");
  surface.commit("\xE3\x81\x8B");
  CHECK(
      surface.takeEvents() ==
      Events{
          "topKeyPress {key:\"\xE3\x81\x8B\",target:2}",
          "topChange {eventCount:6,selection:{end:1,start:1},target:2,text:\"\xE3\x81\x8B\"}",
          "topSelectionChange {selection:{end:1,start:1},target:2}",
      });
}

TEST_CASE("a composition the keyboard empties with no commit leaves the field without it") {
  FocusedField focused("{text: 'a'}");
  auto &surface = focused.surface;
  surface.compose("ka");
  surface.takeEvents();

  surface.compose("");

  CHECK(has(surface.takeEvents(), "topChange {eventCount:4,selection:{end:1,start:1},target:2,text:\"a\"}"));
}

TEST_CASE("a key that types is the field's while text input runs, and the remote's once the field gave the keyboard back") {
  FocusedField focused;
  auto &surface = focused.surface;

  CHECK(surface.press(SDL_SCANCODE_5, SDL_KMOD_NONE, SDLK_5));
  CHECK_FALSE(surface.press(SDL_SCANCODE_5, SDL_KMOD_LCTRL, SDLK_5));
  CHECK_FALSE(surface.press(SDL_SCANCODE_MEDIA_PLAY));
  CHECK(surface.takeEvents().empty());

  surface.command("blur");
  CHECK_FALSE(surface.press(SDL_SCANCODE_5, SDL_KMOD_NONE, SDLK_5));
}

TEST_CASE("Back gives the keyboard back, and never reaches the remote") {
  FocusedField focused("{text: 'a'}");
  auto &surface = focused.surface;

  CHECK(surface.press(SDL_SCANCODE_WEBOS_BACK));

  CHECK(surface.takeEvents() == Events{"topBlur {target:2}", "topEndEditing {target:2,text:\"a\"}"});
}

TEST_CASE("with the on-screen keyboard up, Back lowers it first, OK raises it again, and only then does OK submit") {
  FocusedField focused("{text: 'a'}");
  auto &surface = focused.surface;
  auto &ime = surface.keyboard().ime();
  surface.showKeyboard();

  surface.press(SDL_SCANCODE_WEBOS_BACK);
  CHECK(surface.keyboard().hasFocus(surface.field()));
  CHECK_FALSE(ime.isActive());

  surface.press(SDL_SCANCODE_RETURN);
  CHECK(ime.isActive());
  CHECK(surface.takeEvents().empty());

  surface.press(SDL_SCANCODE_RETURN);
  CHECK(has(surface.takeEvents(), "topSubmitEditing {action:\"submit\",target:2,text:\"a\"}"));
}

TEST_CASE("OK submits after the keyboard went down under a key that types: a hardware keyboard is in use") {
  FocusedField focused("{text: 'a'}");
  auto &surface = focused.surface;
  surface.showKeyboard();
  surface.press(SDL_SCANCODE_B, SDL_KMOD_NONE, SDLK_B);
  SDL_Event hidden{};
  hidden.type = SDL_EVENT_SCREEN_KEYBOARD_HIDDEN;
  surface.keyboard().handleTextEvent(hidden);

  surface.press(SDL_SCANCODE_RETURN);

  CHECK(has(surface.takeEvents(), "topSubmitEditing {action:\"submit\",target:2,text:\"a\"}"));
}

} // namespace rnwebos
