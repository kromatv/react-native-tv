#include "TextInputView.h"

#include "TextInputSurface.test.h"

#include <doctest/doctest.h>

#include <string>
#include <vector>

namespace rnwebos {

namespace {

using testing::Events;
using testing::FocusedField;
using testing::has;
using testing::TextInputSurface;

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

} // namespace rnwebos
