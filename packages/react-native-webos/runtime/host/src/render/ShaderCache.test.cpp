#include "ShaderCache.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace rnwebos {

namespace {

namespace fs = std::filesystem;

struct CacheDirectory {
  CacheDirectory() {
    fs::remove_all(path);
  }
  ~CacheDirectory() {
    fs::remove_all(path);
  }

  fs::path onlyFile() const {
    fs::path found;
    int count = 0;
    for (const auto &entry : fs::directory_iterator(path)) {
      found = entry.path();
      ++count;
    }
    REQUIRE(count == 1);
    return found;
  }

  const fs::path path{fs::temp_directory_path() / "rn-webos-shader-cache-test" / "shaders"};
};

sk_sp<SkData> bytes(const std::string &text) {
  return SkData::MakeWithCopy(text.data(), text.size());
}

std::string text(const sk_sp<SkData> &data) {
  return data ? std::string(static_cast<const char *>(data->data()), data->size()) : "<none>";
}

} // namespace

TEST_CASE_FIXTURE(CacheDirectory, "ShaderCache gives back the program stored under a key, in a later run too") {
  const auto key = bytes("program key");
  {
    ShaderCache cache(path.string());
    CHECK(cache.load(*key) == nullptr);

    cache.store(*key, *bytes("compiled program"), SkString("a program"));

    CHECK(text(cache.load(*key)) == "compiled program");
    CHECK(cache.load(*bytes("another key")) == nullptr);
  }

  ShaderCache nextRun(path.string());

  CHECK(text(nextRun.load(*key)) == "compiled program");
}

TEST_CASE_FIXTURE(CacheDirectory, "ShaderCache replaces the program of a key stored again") {
  ShaderCache cache(path.string());
  const auto key = bytes("program key");

  cache.store(*key, *bytes("first"), SkString());
  cache.store(*key, *bytes("second, longer"), SkString());

  CHECK(text(cache.load(*key)) == "second, longer");
}

TEST_CASE_FIXTURE(CacheDirectory, "ShaderCache misses on a file that is cut short, padded or another key's") {
  ShaderCache cache(path.string());
  const auto key = bytes("program key");
  cache.store(*key, *bytes("compiled program"), SkString());
  const auto file = onlyFile();
  const auto whole = fs::file_size(file);

  fs::resize_file(file, whole - 3);
  CHECK(cache.load(*key) == nullptr);

  cache.store(*key, *bytes("compiled program"), SkString());
  std::ofstream(file, std::ios::binary | std::ios::app) << "more";
  CHECK(cache.load(*key) == nullptr);

  cache.store(*key, *bytes("compiled program"), SkString());
  {
    std::fstream stored(file, std::ios::binary | std::ios::in | std::ios::out);
    stored.seekp(12);
    stored.put('P');
  }
  CHECK(cache.load(*key) == nullptr);

  std::ofstream(file, std::ios::binary | std::ios::trunc) << "junk";
  CHECK(cache.load(*key) == nullptr);
}

TEST_CASE_FIXTURE(CacheDirectory, "ShaderCache keeps no empty program, and nothing once it holds 16 MiB") {
  ShaderCache cache(path.string());
  const auto megabyte = SkData::MakeZeroInitialized(1024 * 1024);

  cache.store(*bytes("empty"), *SkData::MakeEmpty(), SkString());
  for (int index = 0; index < 20; ++index) {
    cache.store(*bytes("program " + std::to_string(index)), *megabyte, SkString());
  }

  CHECK(cache.load(*bytes("empty")) == nullptr);
  CHECK(cache.load(*bytes("program 0")) != nullptr);
  CHECK(cache.load(*bytes("program 14")) != nullptr);
  CHECK(cache.load(*bytes("program 19")) == nullptr);
}

TEST_CASE("ShaderCache keeps nothing in a directory it cannot make") {
  ShaderCache cache("/proc/rn-webos-no-such-directory/shaders");
  const auto key = bytes("program key");

  cache.store(*key, *bytes("compiled program"), SkString());

  CHECK(cache.load(*key) == nullptr);
}

} // namespace rnwebos
