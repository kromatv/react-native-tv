#include "FileIo.h"

#include "ExpoTestRuntime.test.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rntizen::expo {

namespace {

namespace fs = std::filesystem;

fs::path freshDirectory(const char *name) {
  const auto directory = testing::testAppRoot() / "file-io" / name;
  fs::remove_all(directory);
  fs::create_directories(directory);
  return directory;
}

} // namespace

TEST_CASE("readFile reads a whole file, binary bytes included, and nothing that is not there") {
  const auto directory = freshDirectory("read");
  const std::string bytes("a\0b\r\n\xff", 6);
  std::ofstream(directory / "bytes.bin", std::ios::binary) << bytes;
  CHECK(readFile(directory / "bytes.bin") == bytes);
  CHECK(readFile(directory / "empty.txt").has_value() == false);
  std::ofstream(directory / "empty.txt").close();
  CHECK(readFile(directory / "empty.txt") == std::string());
  CHECK_FALSE(readFile(directory / "missing.txt").has_value());
}

TEST_CASE("writeFileAtomically replaces a file whole and leaves no temporary file beside it") {
  const auto directory = freshDirectory("write");
  const auto path = directory / "state.json";
  writeFileAtomically(path, "first");
  writeFileAtomically(path, std::string("second\0", 7));
  CHECK(readFile(path) == std::string("second\0", 7));
  CHECK(std::distance(fs::directory_iterator(directory), fs::directory_iterator()) == 1);
}

TEST_CASE("writeFileAtomically throws a FileWriteError naming the path when it cannot write") {
  const auto directory = freshDirectory("unwritable");
  const auto inMissingDirectory = directory / "missing" / "file.txt";
  CHECK_THROWS_WITH_AS(writeFileAtomically(inMissingDirectory, "data"), ("Cannot write " + inMissingDirectory.string()).c_str(), FileWriteError);

  const auto overDirectory = directory / "taken";
  fs::create_directories(overDirectory / "child");
  CHECK_THROWS_WITH_AS(writeFileAtomically(overDirectory, "data"), ("Cannot write " + overDirectory.string()).c_str(), FileWriteError);
  CHECK(std::distance(fs::directory_iterator(directory), fs::directory_iterator()) == 1);
}

TEST_CASE("localPathOf decodes a file URI's percent escapes and drops its query and fragment") {
  CHECK(localPathOf("file:///data/My%20File%C3%A9.txt") == "/data/My File\xc3\xa9.txt");
  CHECK(localPathOf("file:///data/a.txt?version=2") == "/data/a.txt");
  CHECK(localPathOf("file:///data/a.txt#part") == "/data/a.txt");
  CHECK(localPathOf("file:///data/100%25") == "/data/100%");
  CHECK(localPathOf("file:///data/%zz%4") == "/data/%zz%4");
  CHECK(localPathOf("file:///data/%4a") == "/data/J");
}

TEST_CASE("localPathOf takes a bare absolute path as it is and refuses other schemes") {
  CHECK(localPathOf("/opt/usr/file.txt") == "/opt/usr/file.txt");
  CHECK_FALSE(localPathOf("https://example.com/file.txt").has_value());
  CHECK_FALSE(localPathOf("relative/file.txt").has_value());
  CHECK_FALSE(localPathOf("").has_value());
}

} // namespace rntizen::expo
