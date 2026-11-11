#include "UriBytes.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace rntizen {

namespace {

namespace fs = std::filesystem;

struct TemporaryDirectory {
  TemporaryDirectory() {
    auto pattern = (fs::temp_directory_path() / "rn-tizen-uri-XXXXXX").string();
    if (mkdtemp(pattern.data()) == nullptr) throw std::runtime_error("cannot create a temporary directory");
    path = pattern;
  }
  ~TemporaryDirectory() {
    std::error_code ignored;
    fs::remove_all(path, ignored);
  }
  TemporaryDirectory(const TemporaryDirectory &) = delete;
  TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

  std::string write(const std::string &name, const std::string &bytes) const {
    std::ofstream(path / name, std::ios::binary) << bytes;
    return (path / name).string();
  }

  fs::path path;
};

} // namespace

TEST_CASE("a file: URI reads its file, percent-decoded, with or without localhost") {
  TemporaryDirectory directory;
  const auto path = directory.write("up load.bin", std::string("a\0b", 3));
  const auto encoded = directory.path.string() + "/up%20load.bin";

  CHECK(uriBytes("file://" + encoded) == std::string("a\0b", 3));
  CHECK(uriBytes("file://localhost" + encoded) == std::string("a\0b", 3));
  CHECK(uriBytes("file://" + encoded + "?v=1#top") == std::string("a\0b", 3));
  CHECK(uriBytes("file://" + path) == std::string("a\0b", 3));
  CHECK_FALSE(uriBytes("file://" + directory.path.string() + "/missing").has_value());
  CHECK_FALSE(uriBytes("file://example.com/etc/hosts").has_value());
}

TEST_CASE("a data: URI reads its text percent-decoded, or its base64") {
  CHECK(uriBytes("data:text/plain,hello%20world") == "hello world");
  CHECK(uriBytes("data:,50%%4") == "50%%4");
  CHECK(uriBytes("data:,%zz") == "%zz");
  CHECK(uriBytes("data:text/plain;base64,aGk=") == "hi");
  CHECK(uriBytes("data:;base64,aGVsbG8%3D") == "hello");
  CHECK_FALSE(uriBytes("data:;base64,a*b").has_value());
  CHECK_FALSE(uriBytes("data:no-comma").has_value());
}

TEST_CASE("any other URI has no bytes here") {
  CHECK_FALSE(uriBytes("https://example.com/a.png").has_value());
  CHECK_FALSE(uriBytes("content://media/1").has_value());
  CHECK_FALSE(uriBytes("/plain/path").has_value());
}

} // namespace rntizen
