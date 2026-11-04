#include "EditBuffer.h"

#include <doctest/doctest.h>

#include <string>

namespace rntizen {

TEST_CASE("setText puts the caret after the last code point") {
  EditBuffer buffer;

  buffer.setText("h\xC3\xA9\xF0\x9F\x98\x80");

  CHECK(buffer.size() == 3);
  CHECK(buffer.utf16Size() == 4);
  CHECK(buffer.caret() == 3);
  CHECK_FALSE(buffer.hasSelection());
  CHECK(buffer.selectionUtf16() == EditBuffer::Range{.start = 4, .end = 4});
}

TEST_CASE("insert keeps what maxLength leaves room for and never splits a surrogate pair") {
  EditBuffer buffer;
  buffer.setText("ab");

  const bool changed = buffer.insert(U"c\U0001F600d", 4);

  CHECK(changed);
  CHECK(buffer.utf8() == "abc");
  CHECK(buffer.caret() == 3);
}

TEST_CASE("insert replaces the selection") {
  EditBuffer buffer;
  buffer.setText("hello");
  buffer.selectUtf16(1, 4);

  buffer.insert(U"EY", 100);

  CHECK(buffer.utf8() == "hEYo");
  CHECK(buffer.caret() == 3);
}

TEST_CASE("compose keeps the composition and puts the caret inside it") {
  EditBuffer buffer;
  buffer.setText("ab");
  buffer.moveTo(1, false);

  buffer.compose(U"xyz", 1, 100);

  CHECK(buffer.utf8() == "axyzb");
  CHECK(buffer.composing() == EditBuffer::Range{.start = 1, .end = 4});
  CHECK(buffer.caret() == 2);
}

TEST_CASE("commit replaces the composition and ends it") {
  EditBuffer buffer;
  buffer.compose(U"ka", 2, 100);

  buffer.commit(U"か", 100);

  CHECK(buffer.utf8() == "\xE3\x81\x8B");
  CHECK_FALSE(buffer.composing().has_value());
  CHECK(buffer.caret() == 1);
}

TEST_CASE("deleteBackward and deleteForward delete around the caret") {
  EditBuffer buffer;
  buffer.setText("abc");
  buffer.moveTo(1, false);

  CHECK(buffer.deleteBackward());
  CHECK_FALSE(buffer.deleteBackward());
  CHECK(buffer.deleteForward());

  CHECK(buffer.utf8() == "c");
  CHECK(buffer.caret() == 0);
}

TEST_CASE("deleteAround deletes from an offset after the caret, clamped to the text") {
  EditBuffer buffer;
  buffer.setText("abcdef");
  buffer.moveTo(3, false);

  CHECK(buffer.deleteAround(-2, 2));
  CHECK_FALSE(buffer.deleteAround(10, 1));

  CHECK(buffer.utf8() == "adef");
  CHECK(buffer.caret() == 1);
}

TEST_CASE("selectAll selects the whole text, and typing replaces it") {
  EditBuffer buffer;
  buffer.setText("abc");

  buffer.selectAll();

  CHECK(buffer.text() == U"abc");
  CHECK(buffer.selection() == EditBuffer::Range{.start = 0, .end = 3});
  CHECK(buffer.insert(U"x", 100));
  CHECK(buffer.utf8() == "x");
}

TEST_CASE("moveTo with extend moves only the selection's moving end, and stops at the end of the text") {
  EditBuffer buffer;
  buffer.setText("abcdef");
  buffer.moveTo(1, false);

  buffer.moveTo(4, true);
  CHECK(buffer.selection() == EditBuffer::Range{.start = 1, .end = 4});
  buffer.moveTo(0, true);
  CHECK(buffer.selection() == EditBuffer::Range{.start = 0, .end = 1});
  CHECK(buffer.caret() == 0);
  buffer.moveTo(99, false);

  CHECK(buffer.caret() == 6);
  CHECK_FALSE(buffer.hasSelection());
}

TEST_CASE("selectUtf16 never lands inside a surrogate pair and stops at the end of the text") {
  EditBuffer buffer;
  buffer.setText("a\xF0\x9F\x98\x80z");

  buffer.selectUtf16(2, 99);

  CHECK(buffer.selection() == EditBuffer::Range{.start = 1, .end = 3});
  CHECK(buffer.selectionUtf16() == EditBuffer::Range{.start = 1, .end = 4});
}

TEST_CASE("an insert into a field at its maxLength changes nothing") {
  EditBuffer buffer;
  buffer.setText("abcd");

  CHECK_FALSE(buffer.insert(U"e", 4));

  CHECK(buffer.utf8() == "abcd");
  CHECK(buffer.caret() == 4);
}

TEST_CASE("replacing the selection with the same text is no change, and the caret goes after it") {
  EditBuffer buffer;
  buffer.setText("abc");
  buffer.selectUtf16(0, 1);

  CHECK_FALSE(buffer.insert(U"a", 100));

  CHECK(buffer.utf8() == "abc");
  CHECK(buffer.caret() == 1);
}

TEST_CASE("an empty composition removes the composed text") {
  EditBuffer buffer;
  buffer.setText("x");
  buffer.compose(U"ka", 2, 100);

  CHECK(buffer.compose(U"", 0, 100));

  CHECK(buffer.utf8() == "x");
  CHECK_FALSE(buffer.composing().has_value());
  CHECK(buffer.caret() == 1);
}

TEST_CASE("finishComposing keeps the composed text as typed text") {
  EditBuffer buffer;
  buffer.compose(U"ka", 2, 100);

  buffer.finishComposing();
  buffer.insert(U"!", 100);

  CHECK(buffer.utf8() == "ka!");
  CHECK_FALSE(buffer.composing().has_value());
}

TEST_CASE("deleteBackward and deleteForward delete the selection when there is one") {
  EditBuffer buffer;
  buffer.setText("abcdef");

  buffer.selectUtf16(1, 3);
  CHECK(buffer.deleteBackward());
  buffer.selectUtf16(3, 1);
  CHECK(buffer.deleteForward());

  CHECK(buffer.utf8() == "af");
  CHECK(buffer.caret() == 1);
}

TEST_CASE("deleteForward at the end of the text deletes nothing") {
  EditBuffer buffer;
  buffer.setText("ab");

  CHECK_FALSE(buffer.deleteForward());

  CHECK(buffer.utf8() == "ab");
}

} // namespace rntizen
